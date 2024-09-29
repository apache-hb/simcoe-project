#include "dao/dao.hpp"

#include "base/panic.h"

using namespace sm;
using namespace sm::dao;

std::string_view dao::toString(ColumnType type) noexcept {
    switch (type) {
    case ColumnType::eInt: return "eInt";
    case ColumnType::eUint: return "eUint";
    case ColumnType::eLong: return "eLong";
    case ColumnType::eUlong: return "eUlong";
    case ColumnType::eBool: return "eBool";
    case ColumnType::eString: return "eString";
    case ColumnType::eFloat: return "eFloat";
    case ColumnType::eDouble: return "eDouble";
    case ColumnType::eBlob: return "eBlob";
    default: return "unknown";
    }
}

bool TableInfo::hasPrimaryKey() const noexcept {
    return primaryKey != nullptr;
}

size_t TableInfo::primaryKeyIndex() const noexcept {
    CTASSERT(hasPrimaryKey());
    return std::distance(columns.data(), primaryKey);
}

bool TableInfo::hasUniqueKeys() const noexcept {
    return !uniqueKeys.empty();
}

bool TableInfo::hasForeignKeys() const noexcept {
    return !foreignKeys.empty();
}

const ColumnInfo& TableInfo::getPrimaryKey() const noexcept {
    CTASSERTF(hasPrimaryKey(), "Table %s has no primary key", name.data());
    return *primaryKey;
}

bool TableInfo::hasAutoIncrementPrimaryKey() const noexcept {
    return hasPrimaryKey() && getPrimaryKey().autoIncrement;
}
