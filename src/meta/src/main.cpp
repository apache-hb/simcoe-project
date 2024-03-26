#include "stdafx.hpp"
#include "meta/utils.hpp"
#include "meta/tags.hpp"

using namespace clang::tooling;
using namespace llvm;
using namespace sm;
using namespace clang;

namespace m = meta::detail;
namespace fs = std::filesystem;

static cl::OptionCategory gMetaCategory("meta reflection tool options");

static cl::extrahelp gCommonHelp(CommonOptionsParser::HelpMessage);

namespace {

struct ReflectInfo {
    const clang::Decl& decl;
    std::string name = "";
    std::string category = "";

    std::string brief = "";
    std::string description = "";

    m::Range range = {0, 0};
    bool isTransient = false;

    ReflectInfo(const clang::Decl& decl)
        : decl(decl)
    { }
};

std::string getBriefComment(const comments::FullComment *FC) {
    // TODO: implement
    return "";
}

std::string getDescriptionComment(const comments::FullComment *FC) {
    // TODO: implement
    return "";
}

bool evalTagData(ReflectInfo& Info, Sema& S, const ParsedAttr& Attr) {
    Expr *exprs[15];
    unsigned count = Attr.getNumArgs();
    for (unsigned i = 0; i < count; i++) {
        exprs[i] = Attr.getArgAsExpr(i);
    }

    const AttributeCommonInfo AttrInfo{Attr.getLoc(), Attr.getKind(), Attr.getForm()};
    if (!S.ConstantFoldAttrArgs(AttrInfo, MutableArrayRef<Expr*>(exprs, exprs + count))) {
        return false;
    }

    for (unsigned i = 0; i < count; i++) {
        ConstantExpr *expr = cast<ConstantExpr>(exprs[i]);
        assert(expr && "expected constant expression");

        APValue value = expr->getAPValueResult();
        if (!value.isStruct()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "expected struct value for attribute argument");
            S.Diag(expr->getExprLoc(), ID);
            return false;
        }

        expr->dump();

        unsigned bases = value.getStructNumBases();
        unsigned fields = value.getStructNumFields();
        llvm::outs()
            << "bases: " << bases << "\n"
            << "fields: " << fields << "\n";
    }

    const Decl &D = Info.decl;

    ASTContext& Context = S.getASTContext();
    if (const comments::FullComment* FC = Context.getCommentForDecl(&D, &S.getPreprocessor())) {
        Info.brief = getBriefComment(FC);
        Info.description = getDescriptionComment(FC);
    }

    return true;
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

    return meta::verifyClassIsReflected(S, *RD, Attr);
}

struct MetaMethodAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_method"},
        {ParsedAttr::AS_CXX11, "meta_method"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_method"},
    };

    MetaMethodAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &S, const ParsedAttr &Attr, const Decl *D) const override {
        // This attribute only appertains method
        if (isa<CXXMethodDecl>(D))
            return true;

        S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type_str)
            << Attr << Attr.isRegularKeywordAttribute() << "methods";
        return false;
    }

    AttrHandling handleDeclAttribute(Sema &S, Decl *D, const ParsedAttr &Attr) const override {
        if (!verifyValidReflectContext(S, D, Attr))
            return AttributeNotApplied;

        ReflectInfo Info{*D};
        if (!evalTagData(Info, S, Attr))
            return AttributeNotApplied;

        D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta_method", nullptr, 0, Attr.getRange()));
        return AttributeApplied;
    }
};

struct MetaFieldAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_property"},
        {ParsedAttr::AS_CXX11, "meta_property"},
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
        if (!verifyValidReflectContext(S, D, Attr))
            return AttributeNotApplied;

        ReflectInfo Info{*D};
        if (!evalTagData(Info, S, Attr))
            return AttributeNotApplied;

        // Attach an annotate attribute to the Decl.
        D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta_property", nullptr, 0, Attr.getRange()));

        return AttributeApplied;
    }
};

struct MetaInterfaceAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_interface"},
        {ParsedAttr::AS_Microsoft, "meta_interface"},
        {ParsedAttr::AS_CXX11, "meta_interface"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_interface"},
    };

    MetaInterfaceAttrInfo() {
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
        if (!meta::verifyValidReflectClass(S, *D, Attr))
            return AttributeNotApplied;

        ReflectInfo Info{*D};
        if (!evalTagData(Info, S, Attr))
            return AttributeNotApplied;

        // Attach an annotate attribute to the Decl.
        D->addAttr(AnnotateAttr::Create(S.Context, "simcoe::meta_interface", nullptr, 0, Attr.getRange()));

        return AttributeApplied;
    }
};

struct MetaClassAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta"},
        {ParsedAttr::AS_Microsoft, "meta"},
        {ParsedAttr::AS_CXX11, "meta"},
        {ParsedAttr::AS_CXX11, "simcoe::meta"},
    };

    MetaClassAttrInfo() {
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
        if (!meta::verifyValidReflectClass(S, *D, Attr))
            return AttributeNotApplied;

        ReflectInfo Info{*D};
        if (!evalTagData(Info, S, Attr))
            return AttributeNotApplied;

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

constexpr std::string_view kReflectHeaderContent = R"(
#pragma once
#include "meta/meta.hpp"
using namespace sm::meta;
)";

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
static ParsedAttrInfoRegistry::Add<MetaInterfaceAttrInfo> gInterfaceAttr("meta-interface", "");

// usage: meta [header input] [header output] [source output] -- [compiler options]
int main(int argc, const char **argv) {
    if (argc < 5) {
        llvm::errs() << "Usage: meta [header input] [header output] [source output] -- [compiler options]\n";
        return 1;
    }

    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-Xclang");
    args.push_back("-D__REFLECT__");
    args.push_back("-Xclang");
    args.push_back("-fparse-all-comments");
    args.push_back("-std=c++latest");
    args.push_back(nullptr);
    argc = args.size() - 1;

    std::string error;
    auto options = FixedCompilationDatabase::loadFromCommandLine(argc, args.data(), error);
    if (!options) {
        llvm::errs() << "Failed to parse command line arguments: " << error << "\n";
        return 1;
    }

    auto fs = createOverlayFileSystem();

    const char *input = argv[1];
    const char *headerOutput = argv[2];
    const char *sourceOutput = argv[3];

    ClangTool tool(*options, {input}, std::make_shared<PCHContainerOperations>(), std::move(fs));

    int result = tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
    if (result != 0)
        return result;

    // write the reflection header
    std::ofstream header{headerOutput};
    if (!header.is_open()) {
        llvm::errs() << "Failed to open header output file: " << headerOutput << "\n";
        return 1;
    }

    std::ofstream source{sourceOutput};
    if (!source.is_open()) {
        llvm::errs() << "Failed to open source output file: " << sourceOutput << "\n";
        return 1;
    }

    auto path = fs::path(input).filename();

    const char *note = "// This file is auto generated, do not modify directly\n";

    header << note;
    source << note;

    header << "#pragma once\n\n";
    source << "#include \"" << path.string() << "\"\n";
}
