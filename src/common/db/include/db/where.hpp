#pragma once

#include "dao/dao.hpp"

namespace sm::db {
    class OrderBy {
        const dao::ColumnInfo *mColumn;

    public:
        OrderBy(const dao::ColumnInfo *column) noexcept
            : mColumn(column)
        { }

        const dao::ColumnInfo *column() const noexcept { return mColumn; }
    };

    class GroupBy {
        const dao::ColumnInfo *mColumn;

    public:
        GroupBy(const dao::ColumnInfo *column) noexcept
            : mColumn(column)
        { }

        const dao::ColumnInfo *column() const noexcept { return mColumn; }
    };

    class Limit {
        uint64_t mLimit;

    public:
        Limit(uint64_t limit) noexcept
            : mLimit(limit)
        { }

        uint64_t limit() const noexcept { return mLimit; }
    };

    class Where {

    };
}
