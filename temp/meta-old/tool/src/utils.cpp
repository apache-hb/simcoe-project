#include "stdafx.hpp"
#include "meta/utils.hpp"

using namespace sm;

using namespace clang;

static bool hasReflectTag(const clang::Decl &D, const char *tag) {
    if (D.hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Attr = D.getAttr<AnnotateAttr>();
        if (Attr->getAnnotation().contains(tag)) {
            return true;
        }
    }

    return false;
}

bool meta::isReflectedEnum(const clang::Decl &D) {
    return hasReflectTag(D, kEnumTag);
}

bool meta::isReflectedClass(const clang::Decl &D) {
    return hasReflectTag(D, kClassTag);
}

bool meta::isReflectedInterface(const clang::Decl &D) {
    return hasReflectTag(D, kInterfaceTag);
}

bool meta::isReflectedProperty(const clang::Decl &D) {
    return hasReflectTag(D, kPropertyTag);
}

bool meta::isReflectedMethod(const clang::Decl &D) {
    return hasReflectTag(D, kMethodTag);
}

bool meta::isReflectedCase(const clang::Decl &D) {
    return hasReflectTag(D, kCaseTag);
}

static bool hasVirtualMethods(const CXXRecordDecl &RD) {
    for (const CXXMethodDecl *MD : RD.methods()) {
        if (MD->isVirtual() || MD->isPureVirtual())
            return true;
    }

    return false;
}

static bool isInterface(const clang::Decl &D) {
    if (!isa<CXXRecordDecl>(D))
        return false;

    const CXXRecordDecl &RD = cast<CXXRecordDecl>(D);
    if (!RD.isClass())
        return false;

    if (!RD.hasDefinition())
        return false;

    for (const CXXBaseSpecifier &Base : RD.bases()) {
        auto type = Base.getType();
        if (!isInterface(*type->getAsCXXRecordDecl()))
            return false;
    }

    if (!hasVirtualMethods(RD))
        return false;

    if (!RD.getDestructor()->isVirtual())
        return false;

    return true;
}

bool meta::verifyClassIsReflected(Sema &S, const clang::Decl &D, const ParsedAttr &Attr) {
    if (meta::isReflectedClass(D) || meta::isReflectedInterface(D))
        return true;

    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "class must have 'meta_class' or 'meta_interface' attribute before using '%0'");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
    return false;
}

bool meta::verifyEnumIsReflected(clang::Sema &S, const clang::Decl &D, const clang::ParsedAttr &Attr) {
    if (meta::isReflectedEnum(D))
        return true;

    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "enum must have 'meta_enum' attribute before using '%0'");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
    return false;
}

static bool verifyIsInFileContext(
    clang::DiagnosticsEngine &DE,
    const clang::Decl &D,
    clang::SourceLocation loc,
    const char *tag) {
    if (!D.getDeclContext()->isFileContext()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed at file scope or inside a namespace");
        DE.Report(loc, ID).AddString(tag);
        return false;
    }

    return true;
}

static const CXXRecordDecl *verifyHasDefinition(
    clang::DiagnosticsEngine &DE,
    const clang::Decl &D,
    const char *tag) {

    assert(isa<CXXRecordDecl>(D) && "D must be a CXXRecordDecl");

    const CXXRecordDecl &RD = cast<CXXRecordDecl>(D);
    if (!RD.hasDefinition()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on classes with definitions");
        DE.Report(RD.getLocation(), ID).AddString(tag);
        return nullptr;
    }

    return RD.getDefinition();
}

static bool verifyClassBases(
    clang::DiagnosticsEngine &DE,
    const CXXRecordDecl &RD,
    const char *tag,
    bool (*isReflected)(const NamedDecl &)) {

    if (RD.getNumVBases() != 0) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "reflected class must not have virtual bases");
        DE.Report(RD.getLocation(), ID);
        return false;
    }

    const CXXRecordDecl *lastBaseClass = nullptr;

    for (const CXXBaseSpecifier &Base : RD.bases()) {
        auto type = Base.getType();
        if (!type->isClassType()) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class must be a class");
            DE.Report(RD.getLocation(), ID);
            return false;
        }

        const CXXRecordDecl *BaseRD = type->getAsCXXRecordDecl();
        if (!BaseRD) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class must be a class");
            DE.Report(Base.getBaseTypeLoc(), ID);
            return false;
        }

        if (meta::isReflectedClass(*BaseRD)) {
            if (lastBaseClass != nullptr) {
                unsigned ID = DE.getCustomDiagID(
                    DiagnosticsEngine::Error, "reflected class must have only one base class");
                DE.Report(RD.getLocation(), ID);
                return false;
            }

            lastBaseClass = BaseRD;
        }

        if (!isReflected(*BaseRD)) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class '%0' must be reflected");
            DE.Report(BaseRD->getLocation(), ID).AddString(BaseRD->getName());

            auto note = DE.Report(Base.getSourceRange().getBegin(), diag::note_base_class_specified_here);
            note.AddString(BaseRD->getName());
            return false;
        }
    }

    return true;
}

bool meta::verifyValidReflectInterface(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc) {

    if (!verifyIsInFileContext(DE, D, loc, kInterfaceTag))
        return false;

    const CXXRecordDecl *RD = verifyHasDefinition(DE, D, kInterfaceTag);
    if (!RD) return false;

    if (!isInterface(*RD)) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "reflected interface must be an interface");
        DE.Report(loc, ID);
        return false;
    }

    return verifyClassBases(DE, *RD, kInterfaceTag, [](const NamedDecl &D) {
        return meta::isReflectedInterface(D);
    });
}

bool meta::verifyValidReflectClass(
    clang::DiagnosticsEngine &DE,
    const clang::Decl &D,
    clang::SourceLocation loc) {
    if (!verifyIsInFileContext(DE, D, loc, kClassTag))
        return false;

    const CXXRecordDecl *RD = verifyHasDefinition(DE, D, kClassTag);
    if (!RD) return false;

    if (!RD->isClass()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "reflected class must be a class");
        DE.Report(loc, ID);
        return false;
    }

    return verifyClassBases(DE, *RD, kClassTag, [](const NamedDecl &D) {
        return meta::isReflectedClass(D) || meta::isReflectedInterface(D);
    });
}

bool meta::verifyValidReflectEnum(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc) {

    if (!verifyIsInFileContext(DE, D, loc, kEnumTag))
        return false;

    // make sure the decl is an enum
    if (!isa<EnumDecl>(D)) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on enums");
        DE.Report(loc, ID).AddString(kEnumTag);
        return false;
    }

    return true;
}
