#pragma once

#include "dao/info.hpp"
#include "dao/query.hpp"

namespace sm::dao {
    class ColumnId {
        const ColumnInfo *mColumn;

    public:
        ColumnId(const ColumnInfo *column) noexcept
            : mColumn(column)
        { }

        const ColumnInfo *column() const noexcept { return mColumn; }

        operator QueryExpr() const noexcept;
    };

    template<typename T>
    concept DaoInterface = requires {
        noexcept(T::table());
        { T::table() } -> std::same_as<const TableInfo&>;
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
