#pragma once

#include "dao/column.hpp"

#include <span>
#include <string_view>

namespace sm::dao {
    struct ColumnInfo;
    struct TableInfo;

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
        const OnDelete onDelete;
        const OnUpdate onUpdate;

        constexpr ForeignKey(
            std::string_view name,
            std::string_view column,
            std::string_view foreignTable,
            std::string_view foreignColumn,
            OnDelete onDelete = OnDelete::eRestrict,
            OnUpdate onUpdate = OnUpdate::eRestrict
        ) noexcept
            : ConstraintInfo{name}
            , column{column}
            , foreignTable{foreignTable}
            , foreignColumn{foreignColumn}
            , onDelete{onDelete}
            , onUpdate{onUpdate}
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
