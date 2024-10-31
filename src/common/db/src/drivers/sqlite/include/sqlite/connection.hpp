#pragma once

#include <sqlite3.h>

#include "drivers/common.hpp"

namespace sm::db::sqlite {
    struct CloseDb {
        void operator()(sqlite3 *db) noexcept;
    };

    using Sqlite3Handle = sm::UniquePtr<sqlite3, CloseDb>;

    class SqliteConnection final : public detail::IConnection {
        Sqlite3Handle mConnection;

        sqlite3_stmt *mBeginStmt = nullptr;
        sqlite3_stmt *mCommitStmt = nullptr;
        sqlite3_stmt *mRollbackStmt = nullptr;

        DbError getConnectionError(int err) const noexcept;

        DbError prepare(std::string_view sql, sqlite3_stmt **stmt) noexcept;

        DbError close() noexcept override;

        DbError prepare(std::string_view sql, detail::IStatement **statement) noexcept override;

        DbError begin() noexcept override;
        DbError commit() noexcept override;
        DbError rollback() noexcept override;

        std::string setupInsert(const dao::TableInfo& table) noexcept(false) override;
        std::string setupInsertOrUpdate(const dao::TableInfo& table) noexcept(false) override;
        std::string setupInsertReturningPrimaryKey(const dao::TableInfo& table) noexcept(false) override;

        std::string setupTruncate(const dao::TableInfo& table) noexcept(false) override;

        std::string setupUpdate(const dao::TableInfo& table) noexcept(false) override;

        std::string setupSelect(const dao::TableInfo& table) noexcept(false) override;

        std::string setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) override;

        std::string setupTableExists() noexcept(false) override;

        std::string setupCreateTable(const dao::TableInfo& table) noexcept(false) override;

    public:
        SqliteConnection(Sqlite3Handle connection) noexcept;
    };
}
