#include "stdafx.hpp"
#include "meta/utils.hpp"

using namespace sm;

using namespace clang;

bool meta::isReflectedClass(const clang::Decl &D) {
    if (D.hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Attr = D.getAttr<AnnotateAttr>();
        if (Attr->getAnnotation().starts_with("simcoe::meta")) {
            return true;
        }
    }

    return false;
}

bool meta::isReflectedInterface(const clang::Decl &D) {
    if (D.hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Attr = D.getAttr<AnnotateAttr>();
        if (Attr->getAnnotation().starts_with("simcoe::meta_interface")) {
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

bool meta::canBeReflected(const clang::Decl &D);

bool meta::verifyClassIsReflected(Sema &S, const clang::Decl &D, const ParsedAttr &Attr) {
    if (meta::isReflectedClass(D))
        return true;

    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "class must have 'meta' or 'meta_interface' attribute before using '%0'");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
    return false;
}

bool meta::verifyValidReflectInterface(const clang::Sema &S, const clang::Decl &D, const clang::ParsedAttr &Attr) {
    // Check if the decl is at file scope.
    if (!D.getDeclContext()->isFileContext()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed at file scope or inside a namespace");
        DiagnosticsEngine& DE = S.getDiagnostics();
        DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
        return false;
    }

    if (!meta::isInterface(D)) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on interfaces");
        DiagnosticsEngine& DE = S.getDiagnostics();
        DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
        return false;
    }

    return true;
}

bool meta::verifyValidReflectClass(const clang::Sema &S, const clang::Decl &D, const clang::ParsedAttr &Attr) {
    // Check if the decl is at file scope.
    if (!D.getDeclContext()->isFileContext()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed at file scope or inside a namespace");
        DiagnosticsEngine& DE = S.getDiagnostics();
        DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
        return false;
    }

    // make sure the decl is a class
    const CXXRecordDecl& RD = cast<CXXRecordDecl>(D);
    if (!RD.isClass()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "'%0' attribute only allowed on classes");
        DiagnosticsEngine& DE = S.getDiagnostics();
        DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
        return false;
    }

    return true;
}
