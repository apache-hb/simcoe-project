#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace sm::dao {
    class Column {
        std::string mName;
        std::string mComment;

    public:
        Column(std::string name, std::string comment)
            : mName{std::move(name)}
            , mComment{std::move(comment)}
        { }

        std::string_view name() const noexcept { return mName; }
        std::string_view comment() const noexcept { return mComment; }
    };

    class Table {
        std::string mSchema;
        std::string mName;
        std::string mComment;

        std::vector<Column> mColumns;

    public:
        Table(std::string schema, std::string name, std::string comment, std::vector<Column> columns)
            : mSchema{std::move(schema)}
            , mName{std::move(name)}
            , mComment{std::move(comment)}
            , mColumns{std::move(columns)}
        { }

        std::string_view schema() const noexcept { return mSchema; }
        std::string_view name() const noexcept { return mName; }
        std::string_view comment() const noexcept { return mComment; }
        std::span<const Column> columns() const noexcept { return mColumns; }
    };
}
