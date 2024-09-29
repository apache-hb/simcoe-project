#pragma once

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

    std::string_view toString(ColumnType type) noexcept;

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

    template<typename T>
    concept DaoInterface = requires {
        noexcept(T::getTableInfo());
        { T::getTableInfo() } -> std::same_as<const TableInfo&>;
    };

    template<typename T>
    concept HasPrimaryKey = requires {
        DaoInterface<T>;
        typename T::PrimaryKey;
    };

    namespace detail {
        template<typename T>
        struct TableInfoImpl;
    }
}
