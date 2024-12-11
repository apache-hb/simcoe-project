#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <span>

namespace sm::dao {
    enum class ColumnType : uint_least8_t {
        eInt, // int32
        eUint, // uint32,
        eLong, // int64
        eUlong, // uint64
        eBool, // bool

        /// @note variable length string
        eVarChar, // std::string

        /// @note fixed length string
        eChar,    // std::string

        eFloat, // float
        eDouble, // double
        eBlob, // Blob
        eDateTime, // DateTime
        eDuration, // Duration

        eCount
    };

    constexpr bool isStringType(ColumnType type) noexcept {
        return type == ColumnType::eVarChar || type == ColumnType::eChar;
    }

    enum class AutoIncrement : uint_least8_t {
        eNever, // never auto-increment
        eAlways, // always auto-increment, prevents manual setting
        eDefault, // auto-increment if not set

        eCount
    };

    enum class OnDelete : uint_least8_t {
        eRestrict,
        eSetNull,
        eCascade,

        eCount
    };

    enum class OnUpdate : uint_least8_t {
        eRestrict,
        eCascade,

        eCount
    };

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
