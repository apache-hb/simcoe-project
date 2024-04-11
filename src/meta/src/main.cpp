#include "stdafx.hpp"

#include "meta/utils.hpp"
#include "meta/tags.hpp"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;
using namespace sm;

namespace m = meta::detail;
namespace fs = std::filesystem;

namespace {
cl::OptionCategory gMetaCategory("meta reflection tool options");
cl::extrahelp gCommonHelp(CommonOptionsParser::HelpMessage);

struct Range {
    APValue min;
    APValue max;
};

void addAnnotation(Decl &decl, const ParsedAttr& attr, const char *tag) {
    SmallVector<Expr*, 16> exprs;
    for (unsigned i = 0; i < attr.getNumArgs(); i++) {
        exprs.push_back(attr.getArgAsExpr(i));
    }
    decl.addAttr(AnnotateAttr::Create(decl.getASTContext(), tag, exprs.data(), exprs.size(), attr.getRange()));
}

enum TriState { eFalse, eTrue, eDefault };

void updateTriState(TriState& tri, Sema &S, const clang::Expr& Expr, const char *name, bool value) {
    if (tri == eDefault) {
        tri = value ? eTrue : eFalse;
    } else if ((tri == eTrue) != value) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "conflicting values for '%0'");
        S.Diag(Expr.getExprLoc(), ID) << name;
    }
}

unsigned hashTypeName(const std::string& name) {
    return llvm::hash_value(name);
}

struct ReflectInfo {
    const clang::Decl& decl;

    std::string qualified;
    std::string name;
    std::string category = "";

    std::string brief = "";
    std::string description = "";

    Range range = {APValue(), APValue()};
    TriState isBitFlags = eDefault;
    TriState isTransient = eDefault;
    TriState isThreadSafe = eDefault;
    unsigned typeId = 0;

    ReflectInfo(const clang::Decl& decl)
        : decl(decl)
    {
        if (!isa<clang::NamedDecl>(decl))
            return;

        const clang::NamedDecl& ND = cast<clang::NamedDecl>(decl);
        qualified = ND.getQualifiedNameAsString();
        name = ND.getName();

        typeId = hashTypeName(qualified);
    }

    void setBitFlags(Sema& S, const clang::Expr& Expr, bool value) {
        if (isa<clang::EnumDecl>(decl)) {
            updateTriState(isBitFlags, S, Expr, "bit flags", value);
            return;
        }

        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "bit flags can only be applied to enums");

        S.Diag(decl.getLocation(), ID);
    }

    void setTransient(Sema& S, const clang::Expr& Expr, bool value) {
        if (isa<clang::FieldDecl>(decl)) {
            updateTriState(isTransient, S, Expr, "transient", value);
            return;
        }

        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "transient can only be applied to fields");

        S.Diag(decl.getLocation(), ID);
    }

    void setThreadSafe(Sema& S, const clang::Expr& Expr, bool value) {
        bool ok = isa<clang::CXXMethodDecl>(decl) || isa<clang::FieldDecl>(decl);

        if (!ok) {
            if (isa<clang::CXXRecordDecl>(decl)) {
                const auto& RD = cast<clang::CXXRecordDecl>(decl);
                ok = RD.isClass();
            }
        }

        if (ok) {
            updateTriState(isThreadSafe, S, Expr, "thread safe", value);
            return;
        }

        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "thread safe can only be applied to methods, fields, and classes");

        S.Diag(decl.getLocation(), ID);
    }

    void setTypeId(Sema& S, const clang::Expr& Expr, unsigned id) {
        if (id < 1024) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "typeid must be greater than meta::eUserTypeId (1024). This may happen due to poor luck with hashing, you can specify the typeid manually with typeid = <value>");
            S.Diag(Expr.getExprLoc(), ID);
            return;
        }

        typeId = id;
    }

    void setRange(Sema& S, const clang::Expr& Expr, const APValue& min, const APValue& max) {
        if (!isa<clang::FieldDecl>(decl)) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "range can only be applied to fields");

            S.Diag(decl.getLocation(), ID);
            return;
        }

        auto type = cast<clang::FieldDecl>(decl).getType();

        if (min.isInt() && max.isInt()) {
            int64_t lo = min.getInt().getSExtValue();
            int64_t hi = max.getInt().getSExtValue();

            if (lo > hi) {
                unsigned ID = S.getDiagnostics().getCustomDiagID(
                    DiagnosticsEngine::Error, "range minimum must be less than or equal to maximum");
                S.Diag(Expr.getExprLoc(), ID);
                return;
            }

            if (!type->isIntegerType() && !type->isFloatingType()) {
                unsigned ID = S.getDiagnostics().getCustomDiagID(
                    DiagnosticsEngine::Error, "range must be an integer or floating point type");
                S.Diag(Expr.getExprLoc(), ID);
                return;
            }

            range = Range{min, max};
            return;
        }

        if (min.isFloat() && max.isFloat()) {
            if (!type->isFloatingType()) {
                unsigned ID = S.getDiagnostics().getCustomDiagID(
                    DiagnosticsEngine::Error, "range must be a floating point type");
                S.Diag(Expr.getExprLoc(), ID);
                return;
            }

            double lo = min.getFloat().convertToDouble();
            double hi = max.getFloat().convertToDouble();

            if (lo > hi) {
                unsigned ID = S.getDiagnostics().getCustomDiagID(
                    DiagnosticsEngine::Error, "range minimum must be less than or equal to maximum");
                S.Diag(Expr.getExprLoc(), ID);
                return;
            }

            range = Range{min, max};
            return;
        }
    }
};

std::unordered_map<const clang::Decl*, ReflectInfo> gReflectInfo;

std::string getBriefComment(const comments::FullComment *FC) {
    // TODO: implement
    return "";
}

std::string getDescriptionComment(const comments::FullComment *FC) {
    // TODO: implement
    return "";
}

std::string getStringFromValue(Sema& S, const ConstantExpr& Expr, const APValue& Value) {
    if (!Value.isLValue())
        return "";

    if (Value.isNullPointer())
        return "";

    const auto& base = Value.getLValueBase();
    auto type = base.getType();

    if (!type->isConstantArrayType())
        return "";

    const clang::Expr *E = base.dyn_cast<const clang::Expr*>();
    if (!E)
        return "";

    if (!isa<clang::StringLiteral>(E))
        return "";

    const auto *SL = cast<clang::StringLiteral>(E);

    return SL->getString().str();
}

unsigned getTagFromStruct(Sema& S, const ConstantExpr &Expr, const APValue& value) {
    assert(value.isStruct() && "expected struct value");
    unsigned bases = value.getStructNumBases();
    if (bases == 0) {
        unsigned fields = value.getStructNumFields();
        if (fields != 1) {
            value.dump();
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "tag base struct must have one field, has %0 fields");
            S.Diag(Expr.getExprLoc(), ID) << fields;
            return 0;
        }

        const APValue& field = value.getStructField(0);
        if (!field.isInt()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "tag base struct field must be an integer, got '%0'");
            S.Diag(Expr.getExprLoc(), ID) << field.getKind();
            return 0;
        }

        return field.getInt().getZExtValue();
    } else if (bases == 1) {
        const APValue& base = value.getStructBase(0);
        if (!base.isStruct()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "tag base struct must be a struct");
            S.Diag(Expr.getExprLoc(), ID);
            return 0;
        }

        return getTagFromStruct(S, Expr, base);
    } else {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "tag base struct must have one base");
        S.Diag(Expr.getExprLoc(), ID);
        return 0;
    }
}

std::string getNameFromTag(Sema& S, const ConstantExpr &Expr, const APValue& Value) {
    assert(Value.isStruct() && "expected struct value");
    unsigned fields = Value.getStructNumFields();
    if (fields != 1) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "name tag must have one field");
        S.Diag(Expr.getExprLoc(), ID);
        return "";
    }

    const APValue& field = Value.getStructField(0);
    std::string result = getStringFromValue(S, Expr, field);
    if (!result.empty())
        return result;

    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "name must be a string");
    S.Diag(Expr.getExprLoc(), ID);
    return "";
}

std::string getCategoryFromTag(Sema& S, const ConstantExpr &Expr, const APValue& Value) {
    assert(Value.isStruct() && "expected struct value");
    unsigned fields = Value.getStructNumFields();
    if (fields != 1) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "category tag must have one field");
        S.Diag(Expr.getExprLoc(), ID);
        return "";
    }

    const APValue& field = Value.getStructField(0);
    std::string result = getStringFromValue(S, Expr, field);
    if (!result.empty())
        return result;

    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "category must be a string");
    S.Diag(Expr.getExprLoc(), ID);
    return "";
}

unsigned getTypeIdFromTag(Sema& S, const ConstantExpr &Expr, const APValue& Value) {
    assert(Value.isStruct() && "expected struct value");
    unsigned fields = Value.getStructNumFields();
    if (fields != 1) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "type id tag must have one field");
        S.Diag(Expr.getExprLoc(), ID);
        return 0;
    }

    const APValue& field = Value.getStructField(0);
    if (!field.isInt()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "type id tag field must be an integer");
        S.Diag(Expr.getExprLoc(), ID);
        return 0;
    }

    const APSInt& intVal = field.getInt();
    if (intVal.isSigned() && intVal.isNegative()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "type id tag field must be a positive integer");
        S.Diag(Expr.getExprLoc(), ID);
        return 0;
    }

    if (intVal.getBitWidth() > 32) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "type id tag field must be a 32-bit integer");
        S.Diag(Expr.getExprLoc(), ID);
        return 0;
    }

    return intVal.getZExtValue();
}

void setRangeFromTag(Sema& S, const ConstantExpr &Expr, const APValue& Value, ReflectInfo& Info) {
    assert(Value.isStruct() && "expected struct value");
    unsigned fields = Value.getStructNumFields();
    if (fields != 2) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "range tag must have two fields");
        S.Diag(Expr.getExprLoc(), ID);
        return;
    }

    const APValue& min = Value.getStructField(0);
    const APValue& max = Value.getStructField(1);

    if (min.isInt() && !max.isInt()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "range tag fields must both be the same type");
        S.Diag(Expr.getExprLoc(), ID);
        return;
    }

    if (min.isFloat() && !max.isFloat()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "range tag fields must both be the same type");
        S.Diag(Expr.getExprLoc(), ID);
        return;
    }

    if (!min.isInt() && !min.isFloat()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "range tag fields must be a number");
        S.Diag(Expr.getExprLoc(), ID);
        return;
    }

    if (!max.isInt() && !max.isFloat()) {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "range tag fields must be a number");
        S.Diag(Expr.getExprLoc(), ID);
        return;
    }

    Info.setRange(S, Expr, min, max);
}

bool updateInfoFromTag(ReflectInfo& Info, Sema &S, const ConstantExpr &Expr, unsigned Tag, const APValue& Value) {
    switch (Tag) {
    case m::kTagName:
        Info.name = getNameFromTag(S, Expr, Value);
        return true;
    case m::kTagCategory:
        Info.category = getCategoryFromTag(S, Expr, Value);
        return true;
    case m::kTagRange:
        setRangeFromTag(S, Expr, Value, Info);
        return true;
    case m::kTagBitFlags:
        Info.setBitFlags(S, Expr, true);
        return true;
    case m::kTagTransient:
        Info.setTransient(S, Expr, true);
        return true;
    case m::kTagThreadSafe:
        Info.setThreadSafe(S, Expr, true);
        return true;
    case m::kTagNotThreadSafe:
        Info.setThreadSafe(S, Expr, false);
        return true;
    case m::kTagTypeId:
        Info.setTypeId(S, Expr, getTypeIdFromTag(S, Expr, Value));
        return true;

    default: {
        unsigned ID = S.getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Error, "unknown tag id '%0'");
        DiagnosticsEngine& DE = S.getDiagnostics();
        DE.Report(Expr.getExprLoc(), ID) << Tag;
        return false;
    }
    }
}

bool evalTagData(const Decl *D, Sema& S, const ParsedAttr& Attr) {
    ReflectInfo info(*D);

    SmallVector<Expr*, 15> exprs;
    unsigned count = Attr.getNumArgs();
    for (unsigned i = 0; i < count; i++) {
        exprs.push_back(Attr.getArgAsExpr(i));
    }

    const AttributeCommonInfo AttrInfo{Attr.getLoc(), Attr.getKind(), Attr.getForm()};
    if (!S.ConstantFoldAttrArgs(AttrInfo, exprs)) {
        return false;
    }

    unsigned start = 0;
    if (count >= 1) {
        if (isa<clang::ConstantExpr>(exprs[0])) {
            clang::ConstantExpr *CE = cast<clang::ConstantExpr>(exprs[0]);
            APValue value = CE->getAPValueResult();
            auto name = getStringFromValue(S, *CE, value);
            if (!name.empty()) {
                info.name = name;
                start = 1;
            }
        }
    }

    for (unsigned i = start; i < count; i++) {
        if (!isa<ConstantExpr>(exprs[i]))
            continue;

        ConstantExpr *expr = cast<ConstantExpr>(exprs[i]);

        APValue value = expr->getAPValueResult();
        if (!value.isStruct()) {
            unsigned ID = S.getDiagnostics().getCustomDiagID(
                DiagnosticsEngine::Error, "expected struct value for attribute argument");
            S.Diag(expr->getExprLoc(), ID);
            return false;
        }

        unsigned tag = getTagFromStruct(S, *expr, value);

        if (tag == 0)
            continue;

        if (updateInfoFromTag(info, S, *expr, tag, value))
            continue;
    }

    ASTContext& Context = S.getASTContext();
    if (const comments::FullComment* FC = Context.getCommentForDecl(D, &S.getPreprocessor())) {
        info.brief = getBriefComment(FC);
        info.description = getDescriptionComment(FC);
    }

    gReflectInfo.emplace(D, info);

    return true;
}

void errorNotInClass(Sema &S, const ParsedAttr &Attr) {
    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "'%0' attribute only allowed inside a class");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
}

void errorNotInEnum(Sema &S, const ParsedAttr &Attr) {
    unsigned ID = S.getDiagnostics().getCustomDiagID(
        DiagnosticsEngine::Error, "'%0' attribute only allowed inside an enum");
    DiagnosticsEngine& DE = S.getDiagnostics();
    DE.Report(Attr.getLoc(), ID).AddString(Attr.getAttrName()->getName());
}

// verify the parent context is
// 1. a class
// 2. has the 'simcoe::meta' attribute
bool verifyValidClassContext(Sema &S, const Decl *D, const ParsedAttr &Attr) {
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

bool verifyValidEnumContext(Sema &S, const Decl &D, const ParsedAttr &Attr) {
    const DeclContext *DC = D.getDeclContext();
    if (!isa<EnumDecl>(DC)) {
        errorNotInEnum(S, Attr);
        return false;
    }

    const Decl* E = cast<Decl>(DC);

    return meta::verifyEnumIsReflected(S, *E, Attr);
}

struct MetaCaseAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_case"},
        {ParsedAttr::AS_Microsoft, "meta_case"},
        {ParsedAttr::AS_CXX11, "meta_case"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_case"},
    };

    MetaCaseAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &S, const ParsedAttr &Attr, const Decl *D) const override {
        // This attribute only appertains enums
        if (isa<EnumConstantDecl>(D))
            return true;

        S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type_str)
            << Attr << Attr.isRegularKeywordAttribute() << "enum constants";
        return false;
    }

    AttrHandling handleDeclAttribute(Sema &S, Decl *D, const ParsedAttr &Attr) const override {
        verifyValidEnumContext(S, *D, Attr);
        addAnnotation(*D, Attr, meta::kCaseTag);
        evalTagData(D, S, Attr);
        return AttributeApplied;
    }
};

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
        verifyValidClassContext(S, D, Attr);
        addAnnotation(*D, Attr, meta::kMethodTag);
        evalTagData(D, S, Attr);
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
        verifyValidClassContext(S, D, Attr);
        addAnnotation(*D, Attr, meta::kPropertyTag);
        evalTagData(D, S, Attr);
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
        addAnnotation(*D, Attr, meta::kInterfaceTag);
        evalTagData(D, S, Attr);
        return AttributeApplied;
    }
};

struct MetaClassAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_class"},
        {ParsedAttr::AS_Microsoft, "meta_class"},
        {ParsedAttr::AS_CXX11, "meta_class"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_class"},
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
        addAnnotation(*D, Attr, meta::kClassTag);
        evalTagData(D, S, Attr);
        return AttributeApplied;
    }
};

struct MetaEnumAttrInfo : public ParsedAttrInfo {
    static constexpr Spelling kSpellings[] = {
        {ParsedAttr::AS_GNU, "meta_enum"},
        {ParsedAttr::AS_Microsoft, "meta_enum"},
        {ParsedAttr::AS_CXX11, "meta_enum"},
        {ParsedAttr::AS_CXX11, "simcoe::meta_enum"},
    };

    MetaEnumAttrInfo() {
        OptArgs = 15;
        Spellings = kSpellings;
    }

    bool diagAppertainsToDecl(Sema &S, const ParsedAttr &Attr, const Decl *D) const override {
        // This attribute only appertains enums
        if (isa<EnumDecl>(D))
            return true;

        S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type_str)
            << Attr << Attr.isRegularKeywordAttribute() << "enums";
        return false;
    }

    AttrHandling handleDeclAttribute(Sema &S, Decl *D, const ParsedAttr &Attr) const override {
        addAnnotation(*D, Attr, meta::kEnumTag);
        evalTagData(D, S, Attr);
        return AttributeApplied;
    }
};

std::string gHeaderInputPath;
std::string gHeaderOutPath;
std::string gSourceOutPath;

struct PrettyPrint {
    int depth = 0;

    std::ofstream *of;

    void init(std::ofstream *stream) {
        of = stream;
    }

    void write(const std::string& str) {
        *of << str;
    }

    void println(std::string_view str, auto&&... args) {
        for (int i = 0; i < depth; i++)
            *of << "\t";

        *of << fmt::vformat(str, fmt::make_format_args(args...)) << "\n";
    }

    void indent() { depth++; }
    void dedent() { depth--; }
};

class CmdAfterConsumer : public ASTConsumer {
    clang::DiagnosticsEngine &mDiag;

    std::ofstream mHeaderOutStream;
    std::ofstream mSourceOutStream;

    PrettyPrint mHeaderOut;
    PrettyPrint mSourceOut;

    void handleClass(const Decl &D, const AnnotateAttr &Attr) {
        if (!meta::verifyValidReflectClass(mDiag, D, Attr.getLocation()))
            return;

        // was this declared in the header?
        if (!D.getLocation().isFileID())
            return;

        const SourceManager &SM = D.getASTContext().getSourceManager();
        const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(D.getLocation()));
        if (!FE->tryGetRealPathName().contains(gHeaderInputPath))
            return;

        const ReflectInfo& Info = gReflectInfo.at(&D);
        const CXXRecordDecl &RD = cast<CXXRecordDecl>(D);
        const auto& qualifiedName = RD.getQualifiedNameAsString();

        mHeaderOut.println("#define {}_REFLECT \\\n", RD.getNameAsString());
        mHeaderOut.println("public: \\\n");
        mHeaderOut.println("\tstatic const sm::meta::Class& getClass();\\\n");
        mHeaderOut.println("private: \n");

        mSourceOut.println("template<>\n");
        mSourceOut.println("struct sm::meta::detail::GlobalClass<{}> : public sm::meta::Class {{\n", qualifiedName);
        mSourceOut.indent();
        mSourceOut.println("constexpr GlobalClass() {{\\n");
        mSourceOut.indent();
        mSourceOut.println("setName(\"{}\");\\n", RD.getNameAsString());
        mSourceOut.println("setQualifiedName(\"{}\");\\n", qualifiedName);
        mSourceOut.println("setCategory(\"{}\");\\n", Info.category);
        mSourceOut.println("setTypeId({});\\n", Info.typeId);
        mSourceOut.dedent();
        mSourceOut.println("}};\n");
        mSourceOut.println("static GlobalClass kClass{{}};\n");
        mSourceOut.dedent();
        mSourceOut.println("}};\n");

        mSourceOut.println("const sm::meta::Class& {}::getClass() {{ \n", qualifiedName);
        mSourceOut.indent();
        mSourceOut.println("return sm::meta::detail::GlobalClass<{}>::kClass;\\n", qualifiedName);
        mSourceOut.dedent();
        mSourceOut.println("}}\n");

        for (const CXXBaseSpecifier &Base : RD.bases()) {
            auto type = Base.getType();
            const Decl *BaseRD = type->getAsCXXRecordDecl();
            const ReflectInfo& BaseInfo = gReflectInfo.at(BaseRD);
            llvm::outs() << " - base: " << BaseInfo.qualified << "\n";
        }

        for (const CXXMethodDecl *MD : RD.methods()) {
            if (!meta::isReflectedMethod(*MD))
                continue;

            const ReflectInfo& MethodInfo = gReflectInfo.at(MD);
            llvm::outs() << " - method: " << MethodInfo.qualified << "\n";
        }

        for (const FieldDecl *FD : RD.fields()) {
            if (!meta::isReflectedProperty(*FD))
                continue;

            const ReflectInfo& PropertyInfo = gReflectInfo.at(FD);
            llvm::outs() << " - property: " << PropertyInfo.qualified << "\n";
        }
    }

    void handleInterface(const Decl &D, const AnnotateAttr &Attr) {
        if (!meta::verifyValidReflectInterface(mDiag, D, Attr.getLocation()))
            return;

        // was this declared in the header?
        if (!D.getLocation().isFileID())
            return;

        const SourceManager &SM = D.getASTContext().getSourceManager();
        const FileEntry *FE = SM.getFileEntryForID(SM.getFileID(D.getLocation()));
        if (!FE->tryGetRealPathName().contains(gHeaderInputPath))
            return;

        // const ReflectInfo& Info = gReflectInfo.at(&D);
        const CXXRecordDecl &RD = cast<CXXRecordDecl>(D);

        for (const CXXBaseSpecifier &Base : RD.bases()) {
            auto type = Base.getType();
            const Decl *BaseRD = type->getAsCXXRecordDecl();
            const ReflectInfo& BaseInfo = gReflectInfo.at(BaseRD);
            llvm::outs() << " - base: " << BaseInfo.qualified << "\n";
        }

        for (const CXXMethodDecl *MD : RD.methods()) {
            if (!meta::isReflectedMethod(*MD))
                continue;

            const ReflectInfo& MethodInfo = gReflectInfo.at(MD);
            llvm::outs() << " - method: " << MethodInfo.qualified << "\n";
        }

        for (const FieldDecl *FD : RD.fields()) {
            if (!meta::isReflectedProperty(*FD))
                continue;

            const ReflectInfo& PropertyInfo = gReflectInfo.at(FD);
            llvm::outs() << " - property: " << PropertyInfo.qualified << "\n";
        }
    }

    void handleEnum(const Decl &D, const AnnotateAttr &Attr) {
        if (!meta::verifyValidReflectEnum(mDiag, D, Attr.getLocation()))
            return;
    }

    void Initialize(ASTContext&) override {
        mHeaderOutStream.open(gHeaderOutPath);
        mSourceOutStream.open(gSourceOutPath);

        if (!mHeaderOutStream.is_open()) {
            mDiag.Report(clang::diag::err_cannot_open_file) << gHeaderOutPath;
            return;
        }

        if (!mSourceOutStream.is_open()) {
            mDiag.Report(clang::diag::err_cannot_open_file) << gSourceOutPath;
            return;
        }

        mHeaderOut.init(&mHeaderOutStream);
        mSourceOut.init(&mSourceOutStream);

        mHeaderOut.println("/* This file is generated by the meta reflection tool. */");
        mHeaderOut.println("#pragma once");
        mHeaderOut.println("#include \"meta/class.hpp\"");

        mSourceOut.println("/* This file is generated by the meta reflection tool. */");
        mSourceOut.println("#include \"" + fs::path{gHeaderOutPath}.filename().string() + "\"");
        mSourceOut.println("#include \"" + fs::path{gHeaderInputPath}.filename().string() + "\"");
        mSourceOut.println("#include <array>");

        mHeaderOut.println("namespace sm::meta {{");
        mHeaderOut.indent();
    }

public:
    CmdAfterConsumer(clang::DiagnosticsEngine &Diag)
        : mDiag(Diag)
    { }

    ~CmdAfterConsumer() {
        mHeaderOut.dedent();
        mHeaderOut.println("}}");

        mHeaderOutStream.close();
        mSourceOutStream.close();
    }

    void HandleTranslationUnit(ASTContext &) override {

    }

    bool HandleTopLevelDecl(DeclGroupRef D) override {
        for (Decl *decl : D) {
            if (!decl->hasAttr<AnnotateAttr>())
                continue;

            AnnotateAttr *attr = decl->getAttr<AnnotateAttr>();
            auto id = attr->getAnnotation();
            if (id.contains(meta::kClassTag)) {
                handleClass(*decl, *attr);
            } else if (id.contains(meta::kInterfaceTag)) {
                handleInterface(*decl, *attr);
            } else if (id.contains(meta::kEnumTag)) {
                handleEnum(*decl, *attr);
            }
        }
        return true;
    }
};

class CmdAfterAction : public PluginASTAction {
public:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                    llvm::StringRef) override {
        return std::make_unique<CmdAfterConsumer>(CI.getDiagnostics());
    }

    bool ParseArgs(const CompilerInstance &CI,
                    const std::vector<std::string> &args) override {
        return true;
    }

    PluginASTAction::ActionType getActionType() override {
        return AddAfterMainAction;
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
static ParsedAttrInfoRegistry::Add<MetaCaseAttrInfo> gCaseAttr("meta-case", "");
static ParsedAttrInfoRegistry::Add<MetaEnumAttrInfo> gEnumAttr("meta-enum", "");
static ParsedAttrInfoRegistry::Add<MetaClassAttrInfo> gClassAttr("meta-class", "");
static ParsedAttrInfoRegistry::Add<MetaInterfaceAttrInfo> gInterfaceAttr("meta-interface", "");
static FrontendPluginRegistry::Add<CmdAfterAction> gAfterAction("cmd-after", "");

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

    gHeaderInputPath = fs::path{input}.filename().string();
    gHeaderOutPath = headerOutput;
    gSourceOutPath = sourceOutput;

    ClangTool tool(*options, {input}, std::make_shared<PCHContainerOperations>(), std::move(fs));

    return tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
}
