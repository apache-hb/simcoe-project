#include "stdafx.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

static cl::OptionCategory gMetaCategory("meta reflection tool options");

static cl::extrahelp gCommonHelp(CommonOptionsParser::HelpMessage);

struct ReflectConsumer : public clang::ASTConsumer {
    void HandleTranslationUnit(clang::ASTContext &context) override {
        for (const auto &decl : context.getTranslationUnitDecl()->decls()) {
            if (const auto *record = dyn_cast<clang::CXXRecordDecl>(decl)) {
                if (record->isClass()) {
                    llvm::outs() << "Class: " << record->getNameAsString() << "\n";
                }
            }
        }
    }
};

struct ReflectAction : public clang::SyntaxOnlyAction {
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler,
                                                          llvm::StringRef file) override {
        return std::make_unique<ReflectConsumer>();
    }
};

namespace {

struct MetaFieldAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_property"},
        {ParsedAttr::AS_C23, "simcoe::meta_property"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_property"},
    };

    MetaFieldAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &S, const ParsedAttr &Attr, const Decl *D) const override {
        // This attribute only appertains fields
        if (isa<FieldDecl>(D))
            return true;

        S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type_str)
            << Attr << Attr.isRegularKeywordAttribute() << "fields";
        return false;
    }

    AttrHandling handleDeclAttribute(Sema &S, Decl *D, const ParsedAttr &Attr) const override {
        // Check if the decl is inside a class.
        DeclContext *DC = D->getDeclContext();
        if (!DC->isRecord()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "'simcoe::meta_property' attribute only allowed inside a class");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }

        if (!isa<FieldDecl>(D)) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "'simcoe::meta_property' attribute can only be applied to fields");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }

        FieldDecl *FD = cast<FieldDecl>(D);

        RecordDecl *RD = FD->getParent();
        RD->dump();

        if (!RD->hasAttr<AnnotateAttr>()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "class must have 'simcoe::meta' attribute before using 'simcoe::meta_property'");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }

        AnnotateAttr *Annotate = RD->getAttr<AnnotateAttr>();
        if (!Annotate->getAnnotation().starts_with("simcoe::meta")) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "class must have 'simcoe::meta' annotation before using 'simcoe::meta_property'");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }


        // Attach an annotate attribute to the Decl.
        FD->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta_property", nullptr, 0, Attr.getRange()));

        return AttributeApplied;
    }
};

struct MetaAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta"},
        {ParsedAttr::AS_Microsoft, "simcoe::meta"},
    };

    MetaAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &S, const ParsedAttr &Attr, const Decl *D) const override {
        // This attribute only appertains classes
        if (isa<CXXRecordDecl>(D))
            return true;

        S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type_str)
            << Attr << Attr.isRegularKeywordAttribute() << "classes";
        return false;
    }

    AttrHandling handleDeclAttribute(Sema &S, Decl *D, const ParsedAttr &Attr) const override {
        // Check if the decl is at file scope.
        if (!D->getDeclContext()->isFileContext()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "'simcoe::meta' attribute only allowed at file scope");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }
        // We make some rules here:
        // 1. Only accept at most 3 arguments here.
        // 2. The first argument must be a string literal if it exists.
        if (Attr.getNumArgs() > 3) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error,
                "'simcoe::meta' attribute only accepts at most three arguments");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }
        // If there are arguments, the first argument should be a string literal.
        if (Attr.getNumArgs() > 0) {
            auto *Arg0 = Attr.getArgAsExpr(0);
            clang::StringLiteral *Literal = dyn_cast<clang::StringLiteral>(Arg0->IgnoreParenCasts());
            if (!Literal) {
                unsigned ID = S.getDiagnostics().getCustomDiagID(
                    DiagnosticsEngine::Error, "first argument to the 'simcoe::meta' "
                                              "attribute must be a string literal");
                S.Diag(Attr.getLoc(), ID);
                return AttributeNotApplied;
            }
            SmallVector<Expr *, 16> ArgsBuf;
            for (unsigned i = 0; i < Attr.getNumArgs(); i++) {
                ArgsBuf.push_back(Attr.getArgAsExpr(i));
            }
            D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta", ArgsBuf.data(), ArgsBuf.size(),
                                            Attr.getRange()));
        } else {
            // Attach an annotate attribute to the Decl.
            D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta", nullptr, 0, Attr.getRange()));
        }
        return AttributeApplied;
    }
};

} // namespace

static ParsedAttrInfoRegistry::Add<MetaFieldAttrInfo> Y("meta-property", "");
static ParsedAttrInfoRegistry::Add<MetaAttrInfo> X("meta-decl", "");

// usage: meta [header input] [header output] [source output] -- [compiler options]
int main(int argc, const char **argv) {
    if (argc < 2) {
        llvm::errs() << "Usage: meta [header input] [header output] [source output] -- [compiler options]\n";
        return 1;
    }

    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-Xclang");
    args.push_back("-D__REFLECT__");
    args.push_back(nullptr);
    argc = args.size() - 1;

    std::string error;
    auto options = FixedCompilationDatabase::loadFromCommandLine(argc, args.data(), error);
    if (!options) {
        llvm::errs() << "Failed to parse command line arguments: " << error << "\n";
        return 1;
    }

    ClangTool tool(*options, {argv[1]});

    return tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
}
