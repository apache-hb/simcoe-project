#include "stdafx.hpp"
#include "clang/AST/Decl.h"
#include "clang/Basic/ParsedAttrInfo.h"
#include "clang/Basic/SourceManager.h"

using namespace llvm;
using namespace clang;

namespace fs = std::filesystem;

static constexpr std::string_view kMetaHeaderContent = R"(
#pragma once
#include "meta/meta.hpp"
using namespace sm::meta;
)";

class StubFile final : public vfs::File {
    vfs::Status mStatus;

public:
    StubFile(vfs::Status status)
        : mStatus(std::move(status))
    { }

    ErrorOr<vfs::Status> status() override {
        return mStatus;
    }

    ErrorOr<std::unique_ptr<MemoryBuffer>>
    getBuffer(const Twine &name, int64_t size, bool RequiresNullTerminator, bool IsVolatile) override {
        return MemoryBuffer::getMemBuffer(kMetaHeaderContent, name.getSingleStringRef(), RequiresNullTerminator);
    }

    std::error_code close() override {
        return std::error_code();
    }
};

class ReflectFileSystem final : public vfs::ProxyFileSystem {
    static constexpr uint64_t kUniqueID = 0x12345678;

    vfs::Status mStatus = vfs::Status(
        "stub.meta.hpp",
        sys::fs::UniqueID(0, kUniqueID),
        std::chrono::system_clock::now(),
        0, 0,
        kMetaHeaderContent.size(),
        sys::fs::file_type::regular_file,
        sys::fs::all_all
    );

    static bool isReflectHeader(llvm::StringRef path) {
        return path.ends_with(".meta.hpp");
    }
public:
    ReflectFileSystem()
        : ProxyFileSystem(vfs::getRealFileSystem())
    { }

    ErrorOr<vfs::Status> status(const Twine &path) override {
        if (isReflectHeader(path.str())) {
            return mStatus;
        }

        return ProxyFileSystem::status(path);
    }

    ErrorOr<std::unique_ptr<vfs::File>>
    openFileForRead(const Twine &path) override {
        if (!isReflectHeader(path.str()))
            return ProxyFileSystem::openFileForRead(path);

        return std::make_unique<StubFile>(mStatus);
    }
};

std::unique_ptr<vfs::FileSystem> createOverlayFileSystem() {
    return std::make_unique<ReflectFileSystem>();
}

static fs::path gInputPath;
static fs::path gSourcePath;
static fs::path gHeaderPath;

static constexpr const char *kReflectTag = "meta";

class MetaAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "simco_meta"},
        {ParsedAttr::AS_Microsoft, "simcoe_meta"},
        {ParsedAttr::AS_CXX11, "simcoe::meta"}
    };

    static void addAnnotation(Sema &sema, Decl &decl, const ParsedAttr &attr) {
        SmallVector<Expr*, 16> exprs;
        for (unsigned i = 0; i < attr.getNumArgs(); i++) {
            exprs.push_back(attr.getArgAsExpr(i));
        }

        decl.addAttr(AnnotateAttr::Create(decl.getASTContext(), kReflectTag, exprs.data(), exprs.size(), attr.getRange()));

        const AttributeCommonInfo attrInfo{attr.getLoc(), attr.getKind(), attr.getForm()};
        if (!sema.ConstantFoldAttrArgs(attrInfo, exprs)) {
            return;
        }
    }

    MetaAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &sema, const ParsedAttr &attr, const Decl *decl) const override {
        return true;
    }

    AttrHandling handleDeclAttribute(Sema &sema, Decl *decl, const ParsedAttr &attr) const override {
        CTASSERT(decl != nullptr);

        addAnnotation(sema, *decl, attr);
        return AttributeApplied;
    }
};

class CmdAfterConsumer final : public ASTConsumer {
    static bool shouldSkipDecl(const Decl& decl) {
        if (!decl.getLocation().isFileID())
            return true;

        const SourceManager &sm = decl.getASTContext().getSourceManager();
        if (const FileEntry *entry = sm.getFileEntryForID(sm.getFileID(decl.getLocation()))) {
            return !entry->tryGetRealPathName().contains(gInputPath.string());
        }

        return true;
    }

public:
    CmdAfterConsumer() = default;

    void HandleTagDeclDefinition(TagDecl *decl) override {
        if (shouldSkipDecl(*decl))
            return;
    }
};

class CmdAfterAction final : public PluginASTAction {
    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
        return std::make_unique<CmdAfterConsumer>();
    }

    bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
        return true;
    }

    PluginASTAction::ActionType getActionType() override {
        return AddAfterMainAction;
    }
};

static FrontendPluginRegistry::Add<CmdAfterAction> gAfterAction("cmd-after-action", "Run after action");

int main(int argc, const char **argv) {
    if (argc < 5) {
        llvm::errs() << "Usage: " << argv[0] << " <input> <header output> <source output> -- [options...]\n";
        return 1;
    }

    std::vector<const char*> args(argv, argv + argc);
    args.push_back("-Xclang");
    args.push_back("-D__REFLECT__");
    args.push_back("-std=c++latest");
    args.push_back(nullptr);
    argc = args.size() - 1;

    std::string error;
    std::unique_ptr options = tooling::FixedCompilationDatabase::loadFromCommandLine(argc, args.data(), error);
    if (!options) {
        llvm::errs() << "Failed to load compilation database: " << error << "\n";
        return 1;
    }

    std::unique_ptr fs = createOverlayFileSystem();

    const char *input = argv[1];

    gInputPath = input;
    gHeaderPath = argv[2];
    gSourcePath = argv[3];

    tooling::ClangTool tool(*options, {input}, std::make_shared<PCHContainerOperations>(), std::move(fs));

    return tool.run(tooling::newFrontendActionFactory<SyntaxOnlyAction>().get());
}
