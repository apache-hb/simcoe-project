#include "stdafx.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;

static cl::OptionCategory gMetaCategory("meta reflection tool options");

static cl::extrahelp gCommonHelp(CommonOptionsParser::HelpMessage);

namespace {

bool verifyClassIsReflected(Sema &S, const Decl *D, const ParsedAttr &Attr) {
    if (D->hasAttr<AnnotateAttr>()) {
        const AnnotateAttr *Annotate = D->getAttr<AnnotateAttr>();
        if (Annotate->getAnnotation().starts_with("simcoe::meta")) {
            return true;
        }
    }

    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "class must have 'simcoe::meta' attribute before using '%0'");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
    return false;
}

void errorNotInClass(Sema &S, const ParsedAttr &Attr) {
    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "'%0' attribute only allowed inside a class");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
}

// verify the parent context is
// 1. a class
// 2. has the 'simcoe::meta' attribute
bool verifyValidReflectContext(Sema &S, const Decl *D, const ParsedAttr &Attr) {
    const DeclContext *DC = D->getDeclContext();
    if (!DC->isRecord()) {
        errorNotInClass(S, Attr);
        return false;
    }

    const RecordDecl *RD = cast<RecordDecl>(DC);
    if (!RD->isClass()) {
        errorNotInClass(S, Attr);
        return false;
    }

    return verifyClassIsReflected(S, RD, Attr);
}

struct MetaMethodAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_method"},
        {ParsedAttr::AS_C23, "simcoe::meta_method"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_method"},
    };

    MetaMethodAttrInfo() {
        OptArgs = 1;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &S, const ParsedAttr &Attr, const Decl *D) const override {
        // This attribute only appertains method
        if (isa<CXXMethodDecl>(D))
            return true;

        S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type_str)
            << Attr << Attr.isRegularKeywordAttribute() << "functions";
        return false;
    }

    AttrHandling handleDeclAttribute(Sema &S, Decl *D, const ParsedAttr &Attr) const override {
        if (!verifyValidReflectContext(S, D, Attr))
            return AttributeNotApplied;


        D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta_method", nullptr, 0, Attr.getRange()));
        return AttributeApplied;
    }
};

struct MetaFieldAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_property"},
        {ParsedAttr::AS_C23, "simcoe::meta_property"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_property"},
    };

    MetaFieldAttrInfo() {
        OptArgs = 1;
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
        if (!verifyValidReflectContext(S, D, Attr))
            return AttributeNotApplied;

        // Attach an annotate attribute to the Decl.
        D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta_property", nullptr, 0, Attr.getRange()));

        return AttributeApplied;
    }
};

struct MetaClassAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta"},
        {ParsedAttr::AS_Microsoft, "meta"},
    };

    MetaClassAttrInfo() {
        OptArgs = 1;
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

        CXXRecordDecl *RD = cast<CXXRecordDecl>(D);
        if (!RD->isClass()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "'simcoe::meta' attribute only allowed on classes");
            S.Diag(Attr.getLoc(), ID);
            return AttributeNotApplied;
        }

        // Attach an annotate attribute to the Decl.
        D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta", nullptr, 0, Attr.getRange()));

        return AttributeApplied;
    }
};

///
/// overlay a file system to hide reflection headers from the tool.
/// consuming a reflection header in here would probably cause alot of issues.
/// they also may not exist if we're doing a clean build.
///

constexpr std::string_view kReflectHeaderContent = "#pragma once\n\n";

class StubbedFile final : public vfs::File {
    vfs::Status mStat;

public:
    StubbedFile(vfs::Status Stat)
        : mStat(std::move(Stat))
    { }

    ErrorOr<vfs::Status> status() override {
        return mStat;
    }

    ErrorOr<std::unique_ptr<MemoryBuffer>>
    getBuffer(const Twine &Name, int64_t FileSize, bool RequiresNullTerminator, bool IsVolatile) override {
        return MemoryBuffer::getMemBuffer(kReflectHeaderContent, Name.getSingleStringRef(), RequiresNullTerminator);
    }

    std::error_code close() override {
        return std::error_code();
    }
};

// A file system that hides reflection headers from the compiler
class HiddenReflectFileSystem final : public vfs::ProxyFileSystem {
    vfs::Status mStat;

    static const uint64_t kUniqueID = 0x12345678;

    static bool isReflectHeader(llvm::StringRef Path) {
        return Path.ends_with(".reflect.h");
    }

public:
    HiddenReflectFileSystem()
        : ProxyFileSystem(vfs::getRealFileSystem())
    {
        mStat = vfs::Status("stub.reflect.h", sys::fs::UniqueID(0, kUniqueID), std::chrono::system_clock::now(), 0, 0, kReflectHeaderContent.size(), sys::fs::file_type::regular_file, sys::fs::all_read);
    }


    ErrorOr<vfs::Status> status(const llvm::Twine &Path) override {
        if (!isReflectHeader(Path.str()))
            return ProxyFileSystem::status(Path);

        return mStat;
    }

    llvm::ErrorOr<std::unique_ptr<llvm::vfs::File>>
    openFileForRead(const llvm::Twine &Path) override {
        if (!isReflectHeader(Path.str()))
            return ProxyFileSystem::openFileForRead(Path);

        return std::make_unique<StubbedFile>(mStat);
    }
};

std::unique_ptr<llvm::vfs::FileSystem> createOverlayFileSystem() {
    auto OverlayFS = std::make_unique<llvm::vfs::OverlayFileSystem>(llvm::vfs::getRealFileSystem());
    OverlayFS->pushOverlay(std::make_unique<HiddenReflectFileSystem>());
    return OverlayFS;
}

} // namespace

static ParsedAttrInfoRegistry::Add<MetaMethodAttrInfo> gMethodAttr("meta-method", "");
static ParsedAttrInfoRegistry::Add<MetaFieldAttrInfo> gPropertyAttr("meta-property", "");
static ParsedAttrInfoRegistry::Add<MetaClassAttrInfo> gClassAttr("meta-class", "");

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

    auto fs = createOverlayFileSystem();

    ClangTool tool(*options, {argv[1]}, std::make_shared<PCHContainerOperations>(), fs.get());

    return tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
}
