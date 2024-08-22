#include "stdafx.hpp"

#include "daocc/writer.hpp"

namespace fs = std::filesystem;

using namespace std::string_view_literals;

struct Type {
    enum : int {
        eInt,
        eBool,
        eString,
        eFloat,
        eBlob
    };

    int kind;
    bool nullable;
    size_t size;

    bool isString() const { return kind == eString; }
    bool isBlob() const { return kind == eBlob; }
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

static std::string makeCxxType(const Type& type, bool optional) {
    auto base = [&]() -> std::string {
        switch (type.kind) {
            case Type::eInt: return "int";
            case Type::eBool: return "bool";
            case Type::eString: return "std::string";
            case Type::eFloat: return "float";
            case Type::eBlob: return "std::vector<uint8_t>";
        }

        CT_NEVER("Invalid type %d", (int)type.kind);
    }();

    return optional ? fmt::format("std::optional<{}>", base) : base;
}

static std::string makeSqlType(const Type& type) {
    switch (type.kind) {
        case Type::eInt:
        case Type::eBool: return "INTEGER";
        case Type::eString: return fmt::format("CHARACTER VARYING({})", type.size);
        case Type::eFloat: return "REAL";
        case Type::eBlob: return "BLOB";
    }

    CT_NEVER("Invalid type %d", (int)type.kind);
}

struct ForeignKey {
    std::string column;

    std::string foreignTable;
    std::string foreignColumn;
};

struct Unique {
    std::string name;
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
    std::string schema;
    std::vector<Column> columns;
    std::vector<List> lists;
    std::vector<Unique> unique;
    std::vector<ForeignKey> foregin;
    bool imported = false;

    bool isPrimaryKey(const Column& column) const {
        return primaryKey == column.name;
    }

    std::optional<Column> getPrimaryKey() const {
        for (const auto& column : columns) {
            if (column.name == primaryKey) {
                return column;
            }
        }

        return std::nullopt;
    }

    bool hasPrimaryKey() const {
        return !primaryKey.empty();
    }
};

struct Root {
    std::string name;
    std::vector<Table> tables;
};

using Properties = std::map<std::string, std::string>;

static int gError = 0;

static const std::map<std::string, int> kTypeMap = {
    {"int", Type::eInt},
    {"bool", Type::eBool},
    {"text", Type::eString},
    {"float", Type::eFloat},
    {"blob", Type::eBlob}
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
        fmt::println(stderr, "Unexpected element: {}, expected {}", (char*)node->name, name);
        gError = 1;
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

    gError = 1;
    fmt::println(stderr, "Missing property: {}", key);
    return std::string{def};
}

static Type buildType(const Properties &props) {
    auto type = expectProperty(props, "type", "int");
    bool nullable = getOrDefault(props, "nullable", "true") == "true";
    if (type == "text") {
        auto len = expectProperty(props, "length", "0");

        return Type {
            .kind = Type::eString,
            .nullable = nullable,
            .size = std::stoul(len)
        };
    }

    if (auto it = kTypeMap.find(type); it == kTypeMap.end()) {
        fmt::println(stderr, "Unknown type: {}", type);
        return {};
    }

    return Type {
        .kind = kTypeMap.at(type),
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
            fmt::println(stderr, "Invalid references: {}", references);
            gError = 1;
            return;
        }

        ForeignKey foreign = {
            .column = column.name,
            .foreignTable = split[0],
            .foreignColumn = split[1]
        };

        parent.foregin.push_back(foreign);
        return;
    }

    auto unique = getOrDefault(props, "unique", "");
    if (unique == "true") {
        Unique u = {
            .name = column.name,
            .columns = {column.name}
        };

        parent.unique.push_back(u);
        return;
    }
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
            buildDaoConstraint(parent, column, cur);
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

static void buildTableEntry(Table& parent, xmlNodePtr node) {
    if (nodeIs(node, "column")) {
        auto column = buildDaoColumn(parent, node);
        parent.columns.push_back(column);
    } else if (nodeIs(node, "list")) {
        auto list = buildDaoList(parent, node);
        parent.lists.push_back(list);
    } else {
        fmt::println(stderr, "Unexpected element: {}", (char*)node->name);
        gError = 1;
    }
}

static Table buildDaoTable(xmlNodePtr node) {
    if (!expectNode(node, "table")) {
        return {};
    }

    auto props = getNodeProperties(node);

    Table table = {
        .name = expectProperty(props, "name", ""),
        .primaryKey = getOrDefault(props, "primaryKey", ""),
    };

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            buildTableEntry(table, cur);
        }
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
    Writer header{headerStream};
    Writer source{sourceStream};

    std::ostringstream sqliteStream;
    std::ostringstream orclStream;
    std::ostringstream pgsqlStream;
    Writer sqlite{sqliteStream};
    Writer orcl{orclStream};
    Writer pgsql{pgsqlStream};

    source.writeln("#include \"{}\"", headerPath.filename());

    header.writeln("#pragma once");
    header.writeln();
    header.writeln("#include <string>");
    header.writeln("#include <vector>");
    header.writeln("#include <optional>");
    header.writeln();
    for (auto path : gImportPaths) {
        if (path.filename() == input.filename())
            continue;
        header.writeln("#include \"{}\"", path.replace_extension(".dao.hpp").filename());
    }
    header.writeln();
    header.writeln("namespace {}::dao {{", dao.name);
    header.indent();

    for (size_t i = 0; i < dao.tables.size(); i++) {
        const auto& table = dao.tables[i];
        if (table.imported)
            continue;

        header.writeln();

        auto tableName = singular(pascalCase(table.name));

        bool hasConstraints = table.foregin.size() > 0 || table.unique.size() > 0 || table.hasPrimaryKey();

        header.writeln("/// CREATE TABLE {} (", table.name);
        for (size_t j = 0; j < table.columns.size(); j++) {
            const auto& column = table.columns[j];

            auto sqlType = makeSqlType(column.type);
            header.write("///     {} {}", column.name, sqlType);

            if (!column.optional) {
                header.print(" NOT NULL");
            }

            if (table.isPrimaryKey(column)) {
                if (column.autoIncrement) {
                    header.print(" AUTOINCREMENT");
                }
            }

            if (column.type.isString()) {
                header.print(" CHECK(NOT IS_BLANK_STRING({}))", column.name);
            }

            const char *tail = j + 1 < table.columns.size() ? "," : "";
            if (hasConstraints)
                tail = ",";

            header.print("{}\n", tail);
        }

        if (table.hasPrimaryKey()) {
            header.write("///     CONSTRAINT pk_{} PRIMARY KEY ({})\n", table.name, table.primaryKey);
        }

        for (size_t j = 0; j < table.foregin.size(); j++) {
            const auto& foreign = table.foregin[j];
            auto tail = j + 1 < table.foregin.size() ? "," : "";
            header.write("///     CONSTRAINT fk_{0}_{1}_to_{2}_{3} FOREIGN KEY ({1}) REFERENCES {2}({3}){4}\n", table.name, foreign.column, foreign.foreignTable, foreign.foreignColumn, tail);
        }

        header.writeln("/// );");

        header.writeln("class {} {{", tableName);
        header.indent();

        for (const auto& column : table.columns) {
            auto cxxType = makeCxxType(column.type, column.optional);
            header.writeln("{} m{};", cxxType, pascalCase(column.name));
        }

        header.writeln();

        header.dedent();
        header.writeln("public:");
        header.indent();

        header.write("{}(", tableName);
        for (size_t i = 0; i < table.columns.size(); i++) {
            const auto& column = table.columns[i];
            auto cxxType = makeCxxType(column.type, column.optional);
            header.print("{} {}{}", cxxType, camelCase(column.name), i + 1 < table.columns.size() ? ", " : "");
        }
        header.print(") noexcept\n");
        header.indent();

        for (size_t j = 0; j < table.columns.size(); j++) {
            const auto& column = table.columns[j];
            auto fieldName = pascalCase(column.name);
            auto paramName = camelCase(column.name);
            char sep = (j == 0) ? ':' : ',';
            if (column.type.isMoveConstructed()) {
                header.writeln("{} m{}(std::move({}))", sep, fieldName, paramName);
            } else {
                header.writeln("{} m{}({})", sep, fieldName, paramName);
            }
        }

        header.dedent();
        header.writeln("{{ }}");
        header.writeln();

        for (const auto& column : table.columns) {
            auto fieldName = pascalCase(column.name);
            auto paramName = camelCase(column.name);
            header.writeln("{} {}() const noexcept {{ return m{}; }}", makeCxxType(column.type, column.optional), column.name, fieldName);
        }

        header.dedent();
        header.writeln("}};");
    }

    header.dedent();
    header.writeln("}}");

    return gError;
}
