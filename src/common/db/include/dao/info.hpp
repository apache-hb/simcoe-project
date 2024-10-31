#pragma once

#include <span>
#include <string_view>
#include <vector>
#include <chrono>

namespace sm::db {
    using Blob = std::vector<std::uint8_t>;
    using DateTime = std::chrono::time_point<std::chrono::system_clock>;
    using Duration = std::chrono::milliseconds;
}

namespace sm::dao {
    struct ColumnInfo;
    struct TableInfo;

    enum class ColumnType : uint_least8_t {
        eInt, // int32
        eUint, // uint32,
        eLong, // int64
        eUlong, // uint64
        eBool, // bool
        eString, // std::string

        eFloat, // float
        eDouble, // double
        eBlob, // Blob
        eDateTime, // DateTime
        eDuration, // Duration

        eCount
    };

    enum class AutoIncrement : uint_least8_t {
        eNever, // never auto-increment
        eAlways, // always auto-increment, prevents manual setting
        eDefault, // auto-increment if not set

        eCount
    };

    std::string_view toString(ColumnType type) noexcept;

    struct ColumnInfo {
        std::string_view name;
        std::string_view comment;
        size_t offset;
        size_t length;
        ColumnType type;
        AutoIncrement autoIncrement;
        bool nullable;
    };

    struct ConstraintInfo {
        const std::string_view name;
    };

    struct ForeignKey : ConstraintInfo {
        const std::string_view column;
        const std::string_view foreignTable;
        const std::string_view foreignColumn;

        constexpr ForeignKey(std::string_view name, std::string_view column, std::string_view foreignTable, std::string_view foreignColumn) noexcept
            : ConstraintInfo{name}
            , column{column}
            , foreignTable{foreignTable}
            , foreignColumn{foreignColumn}
        { }
    };

    struct UniqueKey : ConstraintInfo {
        std::span<const size_t> columns;

        constexpr UniqueKey(std::string_view name, std::span<const size_t> columns) noexcept
            : ConstraintInfo{name}
            , columns{columns}
        { }

        template<size_t... I>
        static constexpr UniqueKey ofColumns(std::string_view name) noexcept {
            static constexpr size_t arr[] = {I...};
            return UniqueKey{name, arr};
        }
    };

    struct TableInfo {
        std::string_view schema;
        std::string_view name;
        std::string_view comment;
        const ColumnInfo *primaryKey;

        std::span<const ColumnInfo> columns;

        std::span<const UniqueKey> uniqueKeys;
        std::span<const ForeignKey> foreignKeys;
        bool singleton;

        bool hasPrimaryKey() const noexcept;
        size_t primaryKeyIndex() const noexcept;
        bool hasUniqueKeys() const noexcept;
        bool hasForeignKeys() const noexcept;

        const ColumnInfo& getPrimaryKey() const noexcept;
        bool hasAutoIncrementPrimaryKey() const noexcept;

        bool isSingleton() const noexcept { return singleton; }
    };

}
