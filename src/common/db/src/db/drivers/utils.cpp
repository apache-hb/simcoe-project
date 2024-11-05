#include "stdafx.hpp"

#include "drivers/utils.hpp"

#include "dao/dao.hpp"

#include "core/string.hpp"

namespace detail = sm::db::detail;

size_t detail::primaryKeyIndex(const dao::TableInfo &info) noexcept {
    if (!info.hasPrimaryKey())
        return SIZE_MAX;

    return std::distance(info.columns.data(), info.primaryKey);
}

void detail::cleanErrorMessage(std::string& message) noexcept {
    message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
    std::replace(message.begin(), message.end(), '\n', ' ');
    sm::trimWhitespace(message);
}
