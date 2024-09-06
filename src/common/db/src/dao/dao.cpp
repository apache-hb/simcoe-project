#include "dao/dao.hpp"

#include "base/panic.h"

using namespace sm;
using namespace sm::dao;

const ColumnInfo& TableInfo::getPrimaryKey() const noexcept {
    CTASSERTF(hasPrimaryKey(), "Table %s has no primary key", name.data());
    return columns[primaryKey];
}
