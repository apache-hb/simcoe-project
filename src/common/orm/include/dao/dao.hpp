#pragma once

#include <span>
#include <string_view>

namespace sm::dao {
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

    struct ColumnInfo {
        std::string_view name;
        size_t offset;
        ColumnType type;
    };

    struct TableInfo {
        std::string_view schema;
        std::string_view name;
        std::span<const ColumnInfo> columns;
    };

    template<typename T>
    concept DaoInterface = requires {
        noexcept(T::getTableInfo());
        { T::getTableInfo() } -> std::same_as<const TableInfo&>;
        { T::kTableName } -> std::same_as<const std::string_view&>;
    };

    namespace detail {
        template<typename T>
        struct TableInfoImpl;
    }
}
