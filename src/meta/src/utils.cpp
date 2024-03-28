#include "stdafx.hpp"
#include "meta/utils.hpp"

using namespace sm;

using namespace clang;

bool meta::isReflectedEnum(const clang::Decl &D) {
    if (D.hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Attr = D.getAttr<AnnotateAttr>();
        if (Attr->getAnnotation().contains("meta_enum")) {
            return true;
        }
    }

    return false;
}

bool meta::isReflectedClass(const clang::Decl &D) {
    if (D.hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Attr = D.getAttr<AnnotateAttr>();
        if (Attr->getAnnotation().contains("meta_class")) {
            return true;
        }
    }

    return false;
}

bool meta::isReflectedInterface(const clang::Decl &D) {
    if (D.hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Attr = D.getAttr<AnnotateAttr>();
        if (Attr->getAnnotation().contains("meta_interface")) {
            return true;
        }
    }

    return false;
}

bool meta::isInterface(const clang::Decl &D) {
    if (!isa<CXXRecordDecl>(D))
        return false;

    const CXXRecordDecl &RD = cast<CXXRecordDecl>(D);
    if (RD.isInterfaceLike())
        return true;

    return false;
}

bool meta::canBeReflected(const clang::Decl &D) {
    if (!isa<CXXRecordDecl>(D))
        return false;

    const CXXRecordDecl &RD = cast<CXXRecordDecl>(D);
    if (!RD.hasDefinition())
        return false;

    if (RD.isInterfaceLike())
        return true;

    for (const CXXBaseSpecifier &Base : RD.bases()) {
        const CXXRecordDecl *BaseRD = Base.getType()->getAsCXXRecordDecl();
        if (!BaseRD)
            return false;

        if (!canBeReflected(*BaseRD))
            return false;
    }

    return meta::isReflectedInterface(D) || meta::isReflectedClass(D);
}

bool meta::verifyClassIsReflected(Sema &S, const clang::Decl &D, const ParsedAttr &Attr) {
    if (meta::isReflectedClass(D))
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

bool meta::verifyValidReflectInterface(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc) {
    // Check if the decl is at file scope.
    if (!D.getDeclContext()->isFileContext()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed at file scope or inside a namespace");
        DE.Report(loc, ID).AddString(kInterfaceTag);
        return false;
    }

    if (!meta::isInterface(D)) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on interfaces");
        DE.Report(loc, ID).AddString(kInterfaceTag);
        return false;
    }

    const CXXRecordDecl& RD = cast<CXXRecordDecl>(D);

    if (!RD.hasDefinition()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "class must have a definition");
        DE.Report(loc, ID);
        return false;
    }

    if (RD.getNumVBases() != 0) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "class must not have virtual bases");
        DE.Report(loc, ID);
        return false;
    }

    if (RD.getNumBases() >= 2) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "class must not have more than one base class");
        DE.Report(loc, ID);
        return false;
    }

    for (const CXXBaseSpecifier &Base : RD.bases()) {
        const CXXRecordDecl *BaseRD = Base.getType()->getAsCXXRecordDecl();
        if (!BaseRD) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class must be a class");
            DE.Report(loc, ID);
            return false;
        }

        if (!meta::isReflectedInterface(*BaseRD)) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class '%0' must be reflected");
            DE.Report(loc, ID).AddString(BaseRD->getName());
            return false;
        }
    }

    return true;
}

bool meta::verifyValidReflectClass(
    clang::DiagnosticsEngine &DE,
    const clang::Decl &D,
    clang::SourceLocation loc) {
    // Check if the decl is at file scope.
    if (!D.getDeclContext()->isFileContext()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed at file scope or inside a namespace");
        DE.Report(loc, ID).AddString(kClassTag);
        return false;
    }

    // make sure the decl is a class
    const CXXRecordDecl& RD = cast<CXXRecordDecl>(D);
    if (!RD.isClass()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on classes");
        DE.Report(loc, ID).AddString(kClassTag);
        return false;
    }

    if (!RD.hasDefinition()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "class must have a definition");
        DE.Report(loc, ID);
        return false;
    }

    const CXXRecordDecl *Definition = RD.getDefinition();

    if (Definition->getNumVBases() != 0) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "class must not have virtual bases");
        DE.Report(loc, ID);
        return false;
    }

    if (Definition->getNumBases() >= 2) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "class must not have more than one base class");
        DE.Report(loc, ID);
        return false;
    }

    llvm::outs() << Definition->getName()
        << " has " << Definition->getNumBases() << " bases\n"
        << " has " << Definition->getNumVBases() << " virtual bases\n";

    for (const CXXBaseSpecifier &Base : Definition->bases()) {
        auto type = Base.getType();
        if (!type->isClassType()) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class must be a class");
            DE.Report(loc, ID);
            return false;
        }

        const CXXRecordDecl *BaseRD = type->getAsCXXRecordDecl();
        if (!BaseRD) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class must be a class");
            DE.Report(Base.getBaseTypeLoc(), ID);
            return false;
        }

        if (!meta::isReflectedClass(*BaseRD)) {
            unsigned ID = DE.getCustomDiagID(
                DiagnosticsEngine::Error, "base class '%0' must be reflected");
            DE.Report(BaseRD->getLocation(), ID).AddString(BaseRD->getName());
            return false;
        }
    }

    return true;
}

bool meta::verifyValidReflectEnum(
        clang::DiagnosticsEngine &DE,
        const clang::Decl &D,
        clang::SourceLocation loc) {
    // Check if the decl is at file scope.
    if (!D.getDeclContext()->isFileContext()) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed at file scope or inside a namespace");
        DE.Report(loc, ID).AddString(kEnumTag);
        return false;
    }

    // make sure the decl is an enum
    if (!isa<EnumDecl>(D)) {
        unsigned ID = DE.getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on enums");
        DE.Report(loc, ID).AddString(kEnumTag);
        return false;
    }

    return true;
}
