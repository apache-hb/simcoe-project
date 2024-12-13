#include "dao/dao.hpp"

#include "base/panic.h"

using namespace sm;
using namespace sm::dao;

std::string_view dao::toString(ColumnType type) noexcept {
    switch (type) {
    case ColumnType::eInt: return "INT";
    case ColumnType::eUint: return "UINT";
    case ColumnType::eLong: return "LONG";
    case ColumnType::eUlong: return "ULONG";
    case ColumnType::eBool: return "BOOL";
    case ColumnType::eVarChar: return "VARCHAR";
    case ColumnType::eChar: return "CHAR";
    case ColumnType::eFloat: return "FLOAT";
    case ColumnType::eDouble: return "DOUBLE";
    case ColumnType::eBlob: return "BLOB";
    case ColumnType::eDateTime: return "DATETIME";
    default: return "UNKNOWN";
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
    return hasPrimaryKey() && getPrimaryKey().autoIncrement != AutoIncrement::eNever;
}
