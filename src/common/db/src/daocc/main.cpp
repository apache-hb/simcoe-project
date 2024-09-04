#include "core/string.hpp"
#include "fmt/ranges.h"
#include "stdafx.hpp"

#include "daocc/writer.hpp"
#include "dao/dao.hpp"

namespace fs = std::filesystem;

using namespace std::string_view_literals;

using sm::dao::ColumnType;
using enum ColumnType;
using sm::dao::OnConflict;

struct Type {
    ColumnType kind;
    bool nullable;
    size_t size;

    bool isString() const { return kind == eString; }
    bool isBlob() const { return kind == eBlob; }
    bool isInteger() const { return kind == eInt || kind == eUint || kind == eLong || kind == eUlong; }
    bool isMoveConstructed() const { return isString() || isBlob(); }
};

static std::string pascalCase(std::string_view text) {
    std::string result;
    bool upper = true;
    for (char c : text) {
        if (c == '_') {
            upper = true;
        } else if (upper) {
            result.push_back(std::toupper(c));
            upper = false;
        } else {
            result.push_back(c);
        }
    }

    return result;
}

static std::string camelCase(std::string_view text) {
    std::string result;
    bool upper = false;
    for (char c : text) {
        if (c == '_') {
            upper = true;
        } else if (upper) {
            result.push_back(std::toupper(c));
            upper = false;
        } else {
            result.push_back(c);
        }
    }

    return result;
}

static std::string singular(std::string_view text) {
    if (text.back() == 's') {
        return std::string{text.substr(0, text.size() - 1)};
    }

    return std::string{text};
}

static std::string onConflictToString(OnConflict conflict) noexcept {
    switch (conflict) {
        case OnConflict::eRollback: return "OnConflict::eRollback";
        case OnConflict::eIgnore: return "OnConflict::eIgnore";
        case OnConflict::eReplace: return "OnConflict::eReplace";
        case OnConflict::eAbort: return "OnConflict::eAbort";
    }

    CT_NEVER("Invalid on conflict value: %d", (int)conflict);
}

static std::string makeCxxType(const Type& type, bool optional) {
    auto base = [&]() -> std::string {
        switch (type.kind) {
            case eInt: return "int32_t";
            case eUint: return "uint32_t";
            case eLong: return "int64_t";
            case eUlong: return "uint64_t";
            case eBool: return "bool";
            case eString: return "std::string";
            case eFloat: return "float";
            case eDouble: return "double";
            case eBlob: return "std::vector<uint8_t>";
        }

        CT_NEVER("Invalid type %d", (int)type.kind);
    }();

    return optional ? fmt::format("std::optional<{}>", base) : base;
}

#define STRCASE(id) case id: return #id

static std::string makeColumnType(const Type& type) {
    switch (type.kind) {
    STRCASE(eInt);
    STRCASE(eUint);
    STRCASE(eLong);
    STRCASE(eUlong);
    STRCASE(eBool);
    STRCASE(eString);
    STRCASE(eFloat);
    STRCASE(eDouble);
    STRCASE(eBlob);
    }
}

std::vector<std::string_view> lines(const std::string& text) {
    return sm::splitAll(text, '\n');
}

struct ForeignKey {
    std::string name;
    std::string column;

    std::string foreignTable;
    std::string foreignColumn;
};

struct Unique {
    std::string name;
    OnConflict onConflict = OnConflict::eRollback;
    std::vector<std::string> columns;
};

struct List {
    std::string name;
    std::string of;
    std::string from;
    std::string to;
};

struct Column {
    std::string name;
    Type type;
    bool optional:1;
    bool autoIncrement:1;
};

struct Table {
    std::string name;
    std::string primaryKey;
    OnConflict onConflict = OnConflict::eRollback;
    std::string schema;
    std::vector<Column> columns;
    std::vector<List> lists;
    std::vector<Unique> unique;
    std::vector<ForeignKey> foregin;
    bool imported = false;
    bool syntheticPrimaryKey = false;

    std::vector<size_t> getColumnIndices(const Unique& unique) const noexcept {
        std::vector<size_t> indices;
        indices.reserve(unique.columns.size());
        for (const auto& column : unique.columns) {
            indices.push_back(getColumnIndex(column));
        }

        return indices;
    }

    bool isPrimaryKey(const Column& column) const {
        return primaryKey == column.name;
    }

    size_t getPrimaryKeyIndex() const {
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i].name == primaryKey) {
                return i;
            }
        }

        return SIZE_MAX;
    }

    std::optional<Column> getPrimaryKey() const {
        size_t idx = getPrimaryKeyIndex();
        if (idx == SIZE_MAX) {
            return std::nullopt;
        }

        return columns[idx];
    }

    bool hasPrimaryKey() const {
        return getPrimaryKeyIndex() != SIZE_MAX;
    }

    size_t getColumnIndex(std::string_view name) const {
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i].name == name) {
                return i;
            }
        }

        return SIZE_MAX;
    }

    bool columnExists(std::string_view name) const {
        return getColumnIndex(name) != SIZE_MAX;
    }

};

struct Root {
    std::string name;
    std::vector<Table> tables;
};

using Properties = std::map<std::string, std::string>;

static int gError = 0;

template<typename... A>
static void postError(fmt::format_string<A...> msg, A&&... args) {
    fmt::println(stderr, msg, std::forward<A>(args)...);
    gError = 1;
}

static const std::map<std::string, ColumnType> kTypeMap = {
    {"int", eInt},
    {"uint", eUint},
    {"long", eLong},
    {"ulong", eUlong},
    {"bool", eBool},
    {"text", eString},
    {"float", eFloat},
    {"blob", eBlob}
};

static const std::map<std::string, OnConflict> kOnConflictMap = {
    {"rollback", OnConflict::eRollback},
    {"ignore", OnConflict::eIgnore},
    {"replace", OnConflict::eReplace},
    {"abort", OnConflict::eAbort}
};

static Properties getNodeProperties(xmlNodePtr node) {
    Properties properties;
    for (xmlAttrPtr attr = node->properties; attr; attr = attr->next) {
        properties[(char*)attr->name] = (char*)attr->children->content;
    }

    return properties;
}

static bool nodeIs(xmlNodePtr node, std::string_view name) {
    return name == (const char*)node->name;
}

static bool expectNode(xmlNodePtr node, std::string_view name) {
    if (!nodeIs(node, name)) {
        postError("Unexpected element: {}, expected {}", (char*)node->name, name);
        return false;
    }

    return true;
}

static std::string getOrDefault(const Properties &props, std::string_view key, std::string_view def) {
    if (auto it = props.find(std::string{key}); it != props.end()) {
        return it->second;
    }

    return std::string{def};
}

static std::string expectProperty(const Properties &props, std::string_view key, std::string_view def) {
    if (auto it = props.find(std::string{key}); it != props.end()) {
        return it->second;
    }

    postError("Missing property: {}", key);
    return std::string{def};
}

static std::optional<ColumnType> findType(const std::string& name) {
    if (auto it = kTypeMap.find(name); it != kTypeMap.end()) {
        return it->second;
    }

    return std::nullopt;
}

static Type buildType(const Properties &props) {
    auto type = expectProperty(props, "type", "int");
    bool nullable = getOrDefault(props, "nullable", "true") == "true";
    if (type == "text") {
        auto len = expectProperty(props, "length", "0");

        return Type {
            .kind = ColumnType::eString,
            .nullable = nullable,
            .size = std::stoul(len)
        };
    }

    auto it = findType(type);
    if (!it.has_value()) {
        postError("Unknown type: {}", type);
        return {};
    }

    return Type {
        .kind = it.value(),
        .nullable = nullable,
        .size = 0
    };
}

static std::vector<std::string> splitString(std::string_view text, char sep) {
    std::vector<std::string> result;
    size_t start = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == sep) {
            result.push_back(std::string{text.substr(start, i - start)});
            start = i + 1;
        }
    }

    result.push_back(std::string{text.substr(start)});
    return result;
}

static void buildDaoConstraint(Table& parent, Column& column, xmlNodePtr node) {
    if (!expectNode(node, "constraint")) {
        return;
    }

    auto props = getNodeProperties(node);

    auto references = getOrDefault(props, "references", "");
    if (!references.empty()) {
        auto split = splitString(references, '.');

        if (split.size() != 2) {
            postError("Invalid references: {}", references);
            return;
        }

        auto id = fmt::format("fk_{}_{}_to_{}_{}", parent.name, column.name, split[0], split[1]);
        auto fkName = getOrDefault(props, "name", id);

        ForeignKey foreign = {
            .name = fkName,
            .column = column.name,
            .foreignTable = split[0],
            .foreignColumn = split[1]
        };

        parent.foregin.push_back(foreign);
        return;
    }
}

static OnConflict getConflict(const std::string& name) {
    if (auto it = kOnConflictMap.find(name); it != kOnConflictMap.end()) {
        return it->second;
    }

    postError("Invalid on conflict: {}", name);
    return OnConflict::eRollback;
}

static Column buildDaoColumn(Table& parent, xmlNodePtr node) {
    if (!expectNode(node, "column")) {
        return {};
    }

    auto props = getNodeProperties(node);
    auto name = expectProperty(props, "name", "");
    Column column = {
        .name = name,
        .type = buildType(props),
        .optional = getOrDefault(props, "nullable", "false") == "true",
        .autoIncrement = getOrDefault(props, "autoIncrement", "false") == "true"
    };

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            if (nodeIs(cur, "unique")) {
                auto nodeProps = getNodeProperties(cur);
                auto onConflict = getConflict(getOrDefault(nodeProps, "onConflict", "rollback"));
                auto id = fmt::format("unique_{}_over_{}", parent.name, name);
                Unique u = {
                    .name = getOrDefault(nodeProps, "name", id),
                    .onConflict = onConflict,
                    .columns = {name},
                };
                parent.unique.push_back(u);
            } else {
                buildDaoConstraint(parent, column, cur);
            }
        }
    }

    return column;
}

static List buildDaoList(Table& parent, xmlNodePtr node) {
    if (!expectNode(node, "list")) {
        return {};
    }

    auto props = getNodeProperties(node);
    auto name = expectProperty(props, "name", "");

    List list = {
        .name = name,
        .of = expectProperty(props, "of", ""),
        .from = expectProperty(props, "from", ""),
        .to = expectProperty(props, "to", "")
    };

    return list;
}

static void buildUniqueConstraint(Table& parent, xmlNodePtr node) {
    auto props = getNodeProperties(node);

    std::vector<std::string> columns;

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            if (!nodeIs(cur, "column")) {
                postError("Unexpected element: {}", (char*)cur->name);
                continue;
            }

            std::string name = (char*)cur->children->content;
            if (!parent.columnExists(name)) {
                postError("Table `{}` does not contain column: `{}`", parent.name, name);
                continue;
            }

            columns.push_back(name);
        }
    }

    if (columns.empty()) {
        postError("Unique constraint must contain at least one column");
        return;
    }

    auto onConflict = getConflict(getOrDefault(props, "onConflict", "rollback"));
    auto id = fmt::format("unique_{}_over_{}", parent.name, fmt::join(columns, "_"));
    Unique unique {
        .name = getOrDefault(props, "name", id),
        .onConflict = onConflict,
        .columns = columns
    };

    parent.unique.push_back(unique);
}

static void buildTableEntry(Table& parent, xmlNodePtr node) {
    if (nodeIs(node, "column")) {
        auto column = buildDaoColumn(parent, node);
        parent.columns.push_back(column);
    } else if (nodeIs(node, "list")) {
        auto list = buildDaoList(parent, node);
        parent.lists.push_back(list);
    } else if (nodeIs(node, "unique")) {
        buildUniqueConstraint(parent, node);
    } else {
        postError("Unexpected element: `{}`", (char*)node->name);
    }
}

static Table buildDaoTable(xmlNodePtr node) {
    if (!expectNode(node, "table")) {
        return {};
    }

    auto props = getNodeProperties(node);

    auto onConflict = getConflict(getOrDefault(props, "onConflict", "rollback"));

    Table table = {
        .name = expectProperty(props, "name", ""),
        .primaryKey = getOrDefault(props, "primaryKey", ""),
        .onConflict = onConflict
    };

    auto syntheticPrimaryKey = getOrDefault(props, "syntheticPrimaryKey", "");
    if (!syntheticPrimaryKey.empty()) {
        if (table.hasPrimaryKey()) {
            postError("Table `{}` already contains primary key: `{}`", table.name, table.primaryKey);
        }

        auto type = findType(syntheticPrimaryKey);
        if (!type.has_value()) {
            postError("Invalid syntheticPrimaryKey type: `{}`", syntheticPrimaryKey);
            type = eUlong;
        }

        table.primaryKey = "id";
        table.columns.push_back({
            .name = "id",
            .type = {eUlong, false, 0},
            .optional = false,
            .autoIncrement = true
        });
        table.syntheticPrimaryKey = true;
    }

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            buildTableEntry(table, cur);
        }
    }

    if (table.hasPrimaryKey() && !table.columnExists(table.primaryKey)) {
        postError("Table `{}` does not contain primary key: `{}`", table.name, table.primaryKey);
    }

    return table;
}

static std::vector<Table> buildRootElement(xmlNodePtr node) {
    if (nodeIs(node, "root")) {
        auto name = expectProperty(getNodeProperties(node), "name", "");
        // this is an imported root node
        std::vector<Table> tables;
        for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
            if (cur->type == XML_ELEMENT_NODE) {
                auto inner = buildRootElement(cur);
                tables.insert(tables.end(), inner.begin(), inner.end());
            }
        }

        for (auto& table : tables) {
            table.imported = true;
            table.schema = name;
        }

        return tables;
    }

    return { buildDaoTable(node) };
}

static Root buildDaoRoot(xmlNodePtr node) {
    if (!expectNode(node, "root")) {
        return {};
    }

    auto props = getNodeProperties(node);

    Root ctx = {
        .name = expectProperty(props, "name", "")
    };

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            auto tables = buildRootElement(cur);
            ctx.tables.insert(ctx.tables.end(), tables.begin(), tables.end());
        }
    }

    return ctx;
}

static std::set<fs::path> gImportPaths;

static std::vector<fs::path> gIncludePaths;

static std::set<fs::path> gProcessedFiles;

static std::optional<fs::path> findInclude(const fs::path& path) {
    if (fs::exists(path)) {
        return path;
    }

    for (const auto& include : gIncludePaths) {
        fs::path inc = include / path;
        if (fs::exists(inc)) {
            return inc;
        }
    }

    return {};
}

static int incMatch(const char *uri) {
    return findInclude(uri).has_value();
}

static void *incOpen(const char *uri) {
    fs::path path = findInclude(uri).value();
    gProcessedFiles.emplace(path);
    gImportPaths.insert(path);
    return fopen(path.string().c_str(), "r");
}

static int incClose(void *context) {
    return fclose((FILE*)context);
}

static int incRead(void *context, char *buffer, int size) {
    return fread(buffer, 1, size, (FILE*)context);
}

static void beginArray(Writer& writer, std::string_view name, std::string_view kind, size_t size) {
    writer.writeln("static constexpr std::array<{}, {}> {} = {{", kind, size, name);
    writer.indent();
}

static void endArray(Writer& writer) {
    writer.dedent();
    writer.writeln("}};");
}

static void emitCxxBody(
    const Root& dao, const fs::path& inputPath,
    std::ostream& headerStream, const fs::path& headerPath,
    std::ostream& sourceStream
) {
    Writer header{headerStream};
    Writer source{sourceStream};

    source.writeln("#include \"{}\"", headerPath);

    header.writeln("#pragma once");
    header.writeln();
    header.writeln("#include <string>");
    header.writeln("#include <vector>");
    header.writeln("#include <optional>");
    header.writeln("#include <array>");
    header.writeln();
    header.writeln("#include \"dao/dao.hpp\"");
    header.writeln();
    for (auto path : gImportPaths) {
        if (path.filename() == inputPath)
            continue;
        header.writeln("#include \"{}\"", path.replace_extension(".dao.hpp").filename());
    }
    header.writeln();
    header.writeln("namespace {}::dao {{", dao.name);
    header.indent();

    bool first = true;

    for (size_t i = 0; i < dao.tables.size(); i++) {
        const auto& table = dao.tables[i];
        if (table.imported)
            continue;

        if (!first) {
            header.writeln();
        }

        first = false;

        auto className = singular(pascalCase(table.name));

        source.writeln("template<> struct sm::dao::detail::TableInfoImpl<{}::dao::{}> final {{", dao.name, className);
        source.indent();
        source.writeln("using ClassType = {}::dao::{};", dao.name, className);
        source.writeln("using ColumnType = sm::dao::ColumnType;");

        ///
        /// column info
        ///

        beginArray(source, "kColumns", "ColumnInfo", table.columns.size());
        for (const auto& column : table.columns) {
            auto colType = makeColumnType(column.type);
            source.writeln("ColumnInfo {{");
            source.indent();
            source.writeln(".name = \"{}\",", column.name);
            source.writeln(".offset = offsetof(ClassType, {}),", camelCase(column.name));
            source.writeln(".type = ColumnType::{},", colType);
            source.writeln(".length = {},", column.type.size);
            source.writeln(".autoIncrement = {},", column.autoIncrement ? "true" : "false");
            source.dedent();
            source.writeln("}},");
        }
        endArray(source);
        source.writeln();

        ///
        /// foreign keys
        ///

        beginArray(source, "kForeignKeys", "ForeignKey", table.foregin.size());
        for (const auto& fk : table.foregin) {
            source.writeln("ForeignKey {{");
            source.indent();
            source.writeln("/* name =          */ \"{}\",", fk.name);
            source.writeln("/* column =        */ \"{}\",", fk.column);
            source.writeln("/* foreignTable =  */ \"{}\",", fk.foreignTable);
            source.writeln("/* foreignColumn = */ \"{}\",", fk.foreignColumn);
            source.dedent();
            source.writeln("}},");
        }
        endArray(source);
        source.writeln();

        ///
        /// unique keys
        ///

        beginArray(source, "kUniqueKeys", "UniqueKey", table.unique.size());
        for (const auto& unique : table.unique) {
            auto indices = table.getColumnIndices(unique);
            source.writeln("UniqueKey::ofColumns<{}>(\"{}\", {}),", fmt::join(indices, ", "), unique.name, onConflictToString(unique.onConflict));
        }
        endArray(source);
        source.writeln();


        ///
        /// table info
        ///

        source.writeln("static constexpr TableInfo kTableInfo = {{");
        source.indent();
        source.writeln(".schema      = \"{}\",", dao.name);
        source.writeln(".name        = \"{}\",", table.name);
        source.writeln(".primaryKey  = {}ull,", table.getPrimaryKeyIndex());
        source.writeln(".onConflict  = {},", onConflictToString(table.onConflict));
        source.writeln(".columns     = kColumns,");
        source.writeln(".uniqueKeys  = kUniqueKeys,");
        source.writeln(".foreignKeys = kForeignKeys,");
        source.dedent();
        source.writeln("}};");

        source.dedent();
        source.writeln("}};");
        source.writeln();

        source.writeln("const sm::dao::TableInfo& {}::dao::{}::getTableInfo() noexcept {{", dao.name, className);
        source.indent();
        source.writeln("return sm::dao::detail::TableInfoImpl<{}::dao::{}>::kTableInfo;", dao.name, className);
        source.dedent();
        source.writeln("}}");
        source.writeln();

        header.writeln("struct {} {{", className);
        header.indent();

        if (table.hasPrimaryKey()) {
            auto pk = table.getPrimaryKey().value();
            auto cxxType = makeCxxType(pk.type, pk.optional);
            header.writeln("using Id = {};", cxxType);
        }

        header.writeln("static const sm::dao::TableInfo& getTableInfo() noexcept;");
        header.writeln();

        for (const auto& column : table.columns) {
            auto cxxType = makeCxxType(column.type, column.optional);
            header.writeln("{} {};", cxxType, camelCase(column.name));
        }

        header.dedent();
        header.writeln("}};");
    }

    header.dedent();
    header.writeln("}}");
}

int main(int argc, const char **argv) {
    LIBXML_TEST_VERSION

    if (argc < 8) {
        fmt::println(stderr, "Usage: {} <input> --header <header> --source <source> -d <depfile> <includes...>", argv[0]);
        return 1;
    }

    fs::path input = argv[1];
    fs::path headerPath = argv[3];
    fs::path sourcePath = argv[5];
    fs::path depfile = argv[7];

    gIncludePaths.push_back(input.parent_path());

    for (int i = 8; i < argc; i++) {
        gIncludePaths.push_back(argv[i]);
    }

    if (xmlRegisterInputCallbacks(incMatch, incOpen, incRead, incClose) < 0) {
        fmt::println(stderr, "Failed to register input callbacks");
        return 1;
    }

    xmlDocPtr doc = xmlReadFile(input.string().c_str(), nullptr, XML_PARSE_XINCLUDE | XML_PARSE_NONET);

    if (doc == nullptr) {
        fmt::println(stderr, "Failed to parse {}", input);
        return 1;
    }

    if (xmlXIncludeProcess(doc) == -1) {
        fmt::println(stderr, "Failed to process xinclude");
        return 1;
    }

    xmlDocDump(stdout, doc);

    xmlNodePtr root = xmlDocGetRootElement(doc);
    Root dao = buildDaoRoot(root);

    xmlFreeDoc(doc);

    if (gError != 0) {
        fmt::println(stderr, "Error parsing {}", input);
        return gError;
    }

    {
        std::ofstream depfileStream{depfile, std::ios::out | std::ios::trunc};

        gProcessedFiles.insert(input);

        for (const auto& file : gProcessedFiles) {
            fmt::println(depfileStream, "{}: {}", headerPath, file);
            fmt::println(depfileStream, "{}: {}", sourcePath, file);
        }
    }

    std::ofstream headerStream{headerPath, std::ios::out | std::ios::trunc};
    std::ofstream sourceStream{sourcePath, std::ios::out | std::ios::trunc};

    emitCxxBody(dao, input.filename(), headerStream, headerPath.filename(), sourceStream);

    return gError;
}
