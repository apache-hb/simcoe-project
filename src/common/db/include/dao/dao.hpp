#pragma once

#include "base/panic.h"
#include <span>
#include <string_view>

namespace sm::dao {
    struct ColumnInfo;
    struct TableInfo;

    enum class ColumnType {
        eInt, // int32
        eUint, // uint32,
        eLong, // int64
        eUlong, // uint64
        eBool, // bool
        eString, // std::string

        eFloat,
        eDouble,
        eBlob
    };

    enum class OnConflict {
        eRollback,
        eIgnore,
        eReplace,
        eAbort,
    };

    struct ColumnInfo {
        std::string_view name;
        size_t offset;
        ColumnType type;
        size_t length;
        bool autoIncrement;
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
        const OnConflict onConflict;

        constexpr UniqueKey(std::string_view name, std::span<const size_t> columns, OnConflict onConflict) noexcept
            : ConstraintInfo{name}
            , columns{columns}
            , onConflict{onConflict}
        { }

        template<size_t... I>
        static constexpr UniqueKey ofColumns(std::string_view name, OnConflict onConflict) noexcept {
            static constexpr size_t arr[] = {I...};
            return UniqueKey{name, arr, onConflict};
        }
    };

    struct TableInfo {
        std::string_view schema;
        std::string_view name;
        size_t primaryKey;
        OnConflict onConflict;

        std::span<const ColumnInfo> columns;

        std::span<const UniqueKey> uniqueKeys;
        std::span<const ForeignKey> foreignKeys;

        bool hasPrimaryKey() const noexcept {
            return primaryKey != SIZE_MAX;
        }

        bool hasUniqueKeys() const noexcept {
            return !uniqueKeys.empty();
        }

        bool hasForeignKeys() const noexcept {
            return !foreignKeys.empty();
        }

        const ColumnInfo& getPrimaryKey() const noexcept;

        bool hasAutoIncrementPrimaryKey() const noexcept {
            return hasPrimaryKey() && getPrimaryKey().autoIncrement;
        }
    };

    template<typename T>
    concept DaoInterface = requires {
        noexcept(T::getTableInfo());
        { T::getTableInfo() } -> std::same_as<const TableInfo&>;
    };

    template<typename T>
    concept HasPrimaryKey = requires { typename T::Id; };

    namespace detail {
        template<typename T>
        struct TableInfoImpl;
    }
}
