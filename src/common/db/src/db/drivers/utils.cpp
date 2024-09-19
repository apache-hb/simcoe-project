#include "stdafx.hpp"

#include "drivers/utils.hpp"

#include "dao/dao.hpp"

namespace detail = sm::db::detail;

size_t detail::primaryKeyIndex(const dao::TableInfo &info) noexcept {
    if (!info.hasPrimaryKey())
        return SIZE_MAX;

    return std::distance(info.columns.data(), info.primaryKey);
}
