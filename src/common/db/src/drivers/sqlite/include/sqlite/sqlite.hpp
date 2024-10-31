#pragma once

#include <sqlite3.h>

#include "drivers/common.hpp"

namespace sm::db::sqlite {
    std::string setupCreateTable(const dao::TableInfo& info) noexcept;
    std::string setupCreateSingletonTrigger(std::string_view name) noexcept;
    std::string setupInsert(const dao::TableInfo& info) noexcept;
    std::string setupInsertOrUpdate(const dao::TableInfo& info) noexcept;
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info) noexcept;
    std::string setupUpdate(const dao::TableInfo& info) noexcept;
    std::string setupSelect(const dao::TableInfo& info) noexcept;

    DbError getError(int err) noexcept;
    DbError getError(int err, sqlite3 *db, const char *message = nullptr) noexcept;

    int execStatement(sqlite3_stmt *stmt) noexcept;
}
