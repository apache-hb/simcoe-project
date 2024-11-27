#pragma once

#include <sqlite3.h>

#include "drivers/common.hpp"

namespace sm::db::sqlite {
    std::string setupCreateTable(const dao::TableInfo& info);
    std::string setupCreateSingletonTrigger(std::string_view name);
    std::string setupInsert(const dao::TableInfo& info);
    std::string setupInsertOrUpdate(const dao::TableInfo& info);
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info);
    std::string setupUpdate(const dao::TableInfo& info);
    std::string setupSelect(const dao::TableInfo& info);
    std::string setupSelectByPrimaryKey(const dao::TableInfo& info);

    DbError getError(int err) noexcept;
    DbError getError(int err, sqlite3 *db, const char *message = nullptr) noexcept;

    int execStatement(sqlite3_stmt *stmt) noexcept;
}
