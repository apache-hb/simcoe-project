#include "dao/dao.hpp"

#include "base/panic.h"

using namespace sm;
using namespace sm::dao;

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
