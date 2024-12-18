#include "stdafx.hpp"

#include "writer.hpp"
#include <iostream>

using namespace llvm;
using namespace clang;

namespace fs = std::filesystem;

static constexpr std::string_view kMetaHeaderContent = R"(
#pragma once
#include "meta/meta.hpp"
using namespace sm::reflect::detail;
)";

static std::string getNamespaceAsString(const NamedDecl& decl) {
    std::string name;
    llvm::raw_string_ostream OS(name);
    decl.printNestedNameSpecifier(OS);

    // trim trailing ::
    return OS.str().substr(0, name.size() - 2);
}

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

static constexpr const char *kReflectTag = "meta";

class MetaAttrInfo final : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "simcoe_meta"},
        {ParsedAttr::AS_Microsoft, "simcoe_meta"},
        {ParsedAttr::AS_CXX11, "simcoe_meta"}
    };

    static void addAnnotation(Sema &sema, Decl &decl, const ParsedAttr &attr) {
        SmallVector<Expr*, 16> exprs;
        for (unsigned i = 0; i < attr.getNumArgs(); i++) {
            exprs.push_back(attr.getArgAsExpr(i));
        }

        decl.addAttr(AnnotateAttr::Create(decl.getASTContext(), kReflectTag, exprs.data(), exprs.size(), attr.getRange()));

        const AttributeCommonInfo attrInfo{attr.getLoc(), attr.getKind(), attr.getForm()};
        sema.ConstantFoldAttrArgs(attrInfo, exprs);
    }

    AttrHandling handleDeclAttribute(Sema &sema, Decl *decl, const ParsedAttr &attr) const override {
        CTASSERT(decl != nullptr);

        addAnnotation(sema, *decl, attr);
        return AttributeApplied;
    }

public:
    MetaAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }
};

class CmdAfterConsumer final : public ASTConsumer {
    argo::json mEnums = argo::json::array_e;
    argo::json mClasses = argo::json::array_e;

    static bool isMetaTarget(const Decl& decl) {
        if (!decl.getLocation().isFileID())
            return false;

        const SourceManager &sm = decl.getASTContext().getSourceManager();
        if (const FileEntry *entry = sm.getFileEntryForID(sm.getFileID(decl.getLocation()))) {
            fs::path realpath{entry->tryGetRealPathName().str()};
            if (!fs::equivalent(realpath, gInputPath))
                return false;
        }

        if (const AnnotateAttr *attr = decl.getAttr<AnnotateAttr>()) {
            return attr->getAnnotation() == kReflectTag;
        }

        return false;
    }

    static std::string getEnumeratorName(const clang::EnumConstantDecl& decl) {
        return decl.getNameAsString();
    }

    static std::string getEnumeratorValue(const EnumConstantDecl& decl) {
        llvm::SmallString<32> buffer;
        decl.getInitVal().toString(buffer);
        return std::string(buffer);
    }

    DiagnosticsEngine &mDiag;

    argo::json reflectEnumMember(const EnumConstantDecl& decl) {
        return argo::json::from_object({
            { "name", getEnumeratorName(decl) },
            { "value", getEnumeratorValue(decl) }
        });
    }

    argo::json reflectEnum(const EnumDecl& decl) {
        argo::json members(argo::json::array_e);
        for (const EnumConstantDecl *enumerator : decl.enumerators()) {
            members.append(reflectEnumMember(*enumerator));
        }

        argo::json result = argo::json::from_object({
            { "name", decl.getNameAsString() },
            { "namespace", getNamespaceAsString(decl) },
            { "members", members }
        });

        if (decl.getIntegerTypeRange().isValid()) {
            result.insert("underlying_type", decl.getIntegerType().getAsString());
        }

        return result;
    }

    argo::json reflectRecord(const CXXRecordDecl& decl) {
        auto name = decl.getNameAsString();
        auto ns = getNamespaceAsString(decl);

        return argo::json::from_object({
            { "name", name },
            { "namespace", ns },
            { "is_class", decl.isClass() }
        });
    }

public:
    CmdAfterConsumer(DiagnosticsEngine &diag)
        : mDiag(diag)
    {
        (void)mDiag;
    }

    ~CmdAfterConsumer() {
        argo::json result = argo::json::from_object({
            { "header", gInputPath.string() },
            { "enums", mEnums },
            { "classes", mClasses }
        });

        std::cout << result;
    }

    void HandleTagDeclDefinition(TagDecl *decl) override {
        if (!isMetaTarget(*decl))
            return;

        if (decl->isEnum()) {
            argo::json j = reflectEnum(*cast<EnumDecl>(decl));
            mEnums.append(j);
        } else if (decl->isRecord()) {
            argo::json j = reflectRecord(*cast<CXXRecordDecl>(decl));
            mClasses.append(j);
        } else {
            decl->dump(llvm::errs());
        }
    }
};

class CmdAfterAction final : public PluginASTAction {
    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef InFile) override {
        return std::make_unique<CmdAfterConsumer>(CI.getDiagnostics());
    }

    bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string> &args) override {
        return true;
    }

    PluginASTAction::ActionType getActionType() override {
        return AddAfterMainAction;
    }
};

static ParsedAttrInfoRegistry::Add<MetaAttrInfo> gMetaAction("simcoe-meta-attr", "Reflects a class");
static FrontendPluginRegistry::Add<CmdAfterAction> gAfterAction("cmd-after-action", "Run after action");

enum ArgIndex {
    eInputHeader = 1,

    eArgCount = 2
};

int main(int argc, const char **argv) {
    if (argc < eArgCount) {
        llvm::errs() << "Usage: " << argv[0] << " <input> -- [options...]\n";
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

    const char *input = argv[eInputHeader];

    gInputPath = input;

    tooling::ClangTool tool(*options, {input}, std::make_shared<PCHContainerOperations>(), std::move(fs));

    return tool.run(tooling::newFrontendActionFactory<SyntaxOnlyAction>().get());
}
