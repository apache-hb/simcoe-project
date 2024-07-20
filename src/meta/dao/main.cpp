#include "stdafx.hpp"

#include "writer.hpp"

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
        case Type::eString: return fmt::format("VARCHAR({})", type.size);
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

struct Column {
    std::string name;
    Type type;
    bool optional;
};

struct Table {
    std::string name;
    std::string primaryKey;
    std::vector<Column> columns;
    std::vector<Unique> unique;
    std::vector<ForeignKey> foregin;

    bool isPrimaryKey(const Column& column) const {
        return primaryKey == column.name;
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

static bool expectNode(xmlNodePtr node, std::string_view name) {
    if (name != (const char*)node->name) {
        fmt::println("Unexpected element: {}, expected {}", (char*)node->name, name);
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
    fmt::println("Missing property: {}", key);
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
        fmt::println("Unknown type: {}", type);
        return {};
    }

    return Type {
        .kind = kTypeMap.at(type),
        .nullable = nullable,
        .size = 0
    };
}

static Column buildDaoColumn(xmlNodePtr node) {
    if (!expectNode(node, "column")) {
        return {};
    }

    auto props = getNodeProperties(node);
    auto name = expectProperty(props, "name", "");
    fmt::println("    Column: {}", name);

    return Column {
        .name = name,
        .type = buildType(props)
    };
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

    fmt::println("  Table: {}", table.name);

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            auto column = buildDaoColumn(cur);
            table.columns.push_back(column);
        }
    }

    return table;
}

static Root buildDaoRoot(xmlNodePtr node) {
    if (!expectNode(node, "root")) {
        return {};
    }

    auto props = getNodeProperties(node);

    Root ctx = {
        .name = expectProperty(props, "name", "")
    };

    fmt::println("Root: {}", ctx.name);

    for (xmlNodePtr cur = node->children; cur; cur = cur->next) {
        if (cur->type == XML_ELEMENT_NODE) {
            auto table = buildDaoTable(cur);
            ctx.tables.push_back(table);
        }
    }

    return ctx;
}

int main(int argc, const char **argv) {
    if (argc != 8) {
        fmt::println("Usage: {} <input> --header <header> --source <source> -d <depfile>", argv[0]);
        return 1;
    }

    fs::path input = argv[1];
    fs::path headerPath = argv[3];
    fs::path sourcePath = argv[5];
    fs::path depfile = argv[7];

    xmlDocPtr doc = xmlReadFile(input.string().c_str(), nullptr, XML_PARSE_XINCLUDE);

    if (doc == nullptr) {
        fmt::println("Failed to parse {}", input);
        return 1;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    Root dao = buildDaoRoot(root);

    xmlFreeDoc(doc);

    if (gError != 0) {
        fmt::println("Error parsing {}", input);
        return gError;
    }

    {
        std::ofstream depfileStream{depfile, std::ios::out | std::ios::trunc};
        fmt::println(depfileStream, "{}: {}", headerPath, input);
        fmt::println(depfileStream, "{}: {}", sourcePath, input);
    }

    std::ofstream headerStream{headerPath, std::ios::out | std::ios::trunc};
    std::ofstream sourceStream{sourcePath, std::ios::out | std::ios::trunc};
    Writer header{headerStream};
    Writer source{sourceStream};

    source.writeln("#include \"{}\"", headerPath.filename());

    header.writeln("#pragma once");
    header.writeln();
    header.writeln("#include <string>");
    header.writeln("#include <vector>");
    header.writeln("#include <optional>");
    header.writeln();
    header.writeln("namespace {}::dao {{", dao.name);
    header.indent();

    for (size_t i = 0; i < dao.tables.size(); i++) {
        if (i != 0) {
            header.writeln();
        }

        const auto& table = dao.tables[i];
        auto tableName = singular(pascalCase(table.name));

        header.writeln("/// CREATE TABLE {} (", table.name);
        for (size_t j = 0; j < table.columns.size(); j++) {
            const auto& [name, type, optional] = table.columns[j];

            auto sqlType = makeSqlType(type);
            header.write("/// \t{} {}", name, sqlType);

            if (table.isPrimaryKey(table.columns[j])) {
                header.print(" PRIMARY KEY");
            }

            if (!optional) {
                header.print(" NOT NULL");
            }

            if (type.isString()) {
                header.print(" CHECK(NOT IS_BLANK_STRING({}))", name);
            }

            header.print("{}", j + 1 < table.columns.size() ? "," : "");
            header.print("\n");
        }
        header.writeln("/// )");

        header.writeln("class {} {{", tableName);
        header.indent();

        for (const auto& [name, type, optional] : table.columns) {
            auto cxxType = makeCxxType(type, optional);
            header.writeln("{} m{};", cxxType, pascalCase(name));
        }

        header.writeln();

        header.dedent();
        header.writeln("public:");
        header.indent();

        header.write("{}(", tableName);
        for (size_t i = 0; i < table.columns.size(); i++) {
            const auto& [name, type, optional] = table.columns[i];
            auto cxxType = makeCxxType(type, optional);
            header.print("{} {}{}", cxxType, camelCase(name), i + 1 < table.columns.size() ? ", " : "");
        }
        header.print(") noexcept\n");
        header.indent();

        for (size_t j = 0; j < table.columns.size(); j++) {
            const auto& [name, type, optional] = table.columns[j];
            auto fieldName = pascalCase(name);
            auto paramName = camelCase(name);
            char sep = (j == 0) ? ':' : ',';
            if (type.isMoveConstructed()) {
                header.writeln("{} m{}(std::move({}))", sep, fieldName, paramName);
            } else {
                header.writeln("{} m{}({})", sep, fieldName, paramName);
            }
        }

        header.dedent();
        header.writeln("{{ }}");
        header.writeln();

        for (const auto& [name, type, optional] : table.columns) {
            auto fieldName = pascalCase(name);
            auto paramName = camelCase(name);
            header.writeln("{} {}() const noexcept {{ return m{}; }}", makeCxxType(type, optional), name, fieldName);
        }

        header.dedent();
        header.writeln("}};");
    }

    header.dedent();
    header.writeln("}}");

    return gError;
}
