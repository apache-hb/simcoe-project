#include "stdafx.hpp"

#define META_NOCOPY(cls) cls(const cls&) = delete; cls& operator=(const cls&) = delete

static constexpr CXTranslationUnit_Flags kFlags
    = CXTranslationUnit_SkipFunctionBodies;

struct ClangString {
    CXString str;

    META_NOCOPY(ClangString);

    ClangString(CXString str) : str(str) {}
    ~ClangString() { clang_disposeString(str); }

    operator const char*() const { return clang_getCString(str); }
    const char *c_str() const { return clang_getCString(str); }
};

struct ClangCursor {
    CXCursor cursor;

    ClangCursor(CXCursor cursor) : cursor(cursor) {}

    std::vector<ClangCursor> children() const {
        std::vector<ClangCursor> result;

        clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
            std::vector<ClangCursor> *result = static_cast<std::vector<ClangCursor>*>(client_data);
            result->emplace_back(cursor);
            return CXChildVisit_Continue;
        }, &result);

        return result;
    }

    CXCursorKind kind() const {
        return clang_getCursorKind(cursor);
    }

    ClangString spelling() const {
        return clang_getCursorSpelling(cursor);
    }
};

struct MetaClass {
    std::string name;
};

struct Context {
    std::vector<MetaClass> classes;

    void build_metaclass(ClangCursor cursor) {
        auto children = cursor.children();
        if (children.empty()) {
            return;
        }

        auto first = children.front();
        if (first.kind() != CXCursor_AnnotateAttr) {
            return;
        }

        ClangString annotation = first.spelling();

        if (std::strncmp(annotation.c_str(), "reflect@", 8) != 0) {
            return;
        }

        (void)std::printf("found reflected class: %s\n", first.spelling().c_str());

        ClangString name = cursor.spelling();
        MetaClass cls = {name.c_str()};

        children.erase(children.begin());

        for (const ClangCursor &child : children) {
            CXCursorKind kind = child.kind();
            if (kind != CXCursor_FieldDecl) {
                continue;
            }

            auto children = child.children();
            for (const ClangCursor &inner : children) {
                CXCursorKind kind = inner.kind();
                if (kind != CXCursor_AnnotateAttr) {
                    continue;
                }

                (void)std::printf("found reflected field: %s\n", child.spelling().c_str());
            }
        }

        classes.push_back(cls);
    }

    CXChildVisitResult accept(ClangCursor cursor, CXCursor parent) {
        CXCursorKind kind = cursor.kind();
        if (kind == CXCursor_Namespace) {
            return CXChildVisit_Recurse;
        }

        if (kind == CXCursor_ClassDecl) {
            build_metaclass(cursor);
        }

        return CXChildVisit_Continue;
    }
};

// usage: meta <input header> <output header> <output source> <output json> -- <clang args>
int main(int argc, const char **argv) {
    if (argc < 5) {
        (void)std::fprintf(stderr, "usage: meta <input header> <output header> <output source> <output json> -- <clang args>\n");
        return 1;
    }

    const char *input = argv[1];
    // const char *output_header = argv[2];
    // const char *output_source = argv[3];
    // const char *output_json = argv[4];

    std::vector<const char*> args;
    args.push_back("-x");
    args.push_back("c++");
    args.push_back("-D__REFLECT__");
    args.insert(args.end(), argv + 5, argv + argc);

    CXIndex index = clang_createIndex(0, 1);
    CXTranslationUnit tu = clang_parseTranslationUnit(index, input, args.data(), args.size(), nullptr, 0, kFlags);

    if (tu == nullptr) {
        (void)std::fprintf(stderr, "error: failed to parse translation unit\n");
        return 1;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(tu);

    Context context;

    clang_visitChildren(cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
        Context *context = static_cast<Context*>(client_data);
        return context->accept(cursor, parent);
    }, &context);

    clang_disposeTranslationUnit(tu);

    for (const MetaClass &cls : context.classes) {
        (void)std::printf("class: %s\n", cls.name.c_str());
    }
}
