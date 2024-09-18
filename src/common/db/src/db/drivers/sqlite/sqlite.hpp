#pragma once

#include "drivers/common.hpp"

namespace sm::db::detail::sqlite {
    static void checkError(const DbError& err) noexcept {
        CTASSERTF(err.isSuccess(), "Error: %d (%s)", err.code(), err.message().data());
    }

    #define CHECK_ERROR(expr) checkError(#expr, expr)

    std::string setupCreateTable(const dao::TableInfo& info) noexcept;
    std::string setupInsert(const dao::TableInfo& info) noexcept;
    std::string setupInsertOrUpdate(const dao::TableInfo& info) noexcept;
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info) noexcept;
    std::string setupUpdate(const dao::TableInfo& info) noexcept;
    std::string setupSelect(const dao::TableInfo& info) noexcept;

    DbError getError(int err) noexcept;
    DbError getError(int err, sqlite3 *db) noexcept;

    int execStatement(sqlite3_stmt *stmt) noexcept;

    class SqliteStatement final : public detail::IStatement {
        sqlite3_stmt *mStatement = nullptr;
        int mStatus = SQLITE_OK;

        DbError getStmtError(int err) const noexcept;

    public:

        DbError finalize() noexcept override;
        DbError start(bool autoCommit, StatementType type) noexcept override;
        DbError execute() noexcept override;
        DbError next() noexcept override;


        int getBindCount() const noexcept override;

        DbError getBindIndex(std::string_view name, int& index) const noexcept override;

        DbError bindIntByIndex(int index, int64 value) noexcept override;
        DbError bindBooleanByIndex(int index, bool value) noexcept override;
        DbError bindStringByIndex(int index, std::string_view value) noexcept override;
        DbError bindDoubleByIndex(int index, double value) noexcept override;
        DbError bindBlobByIndex(int index, Blob value) noexcept override;
        DbError bindNullByIndex(int index) noexcept override;

        DbError update(bool autoCommit) noexcept override;

        int getColumnCount() const noexcept override;
        DbError getColumnIndex(std::string_view name, int& index) const noexcept override;

        DbError getColumnInfo(int index, ColumnInfo& info) const noexcept override;


        bool isRowReady() const noexcept;

        DbError rowNotReady() const noexcept;

        DbError getIntByIndex(int index, int64& value) noexcept override;
        DbError getBooleanByIndex(int index, bool& value) noexcept override;
        DbError getStringByIndex(int index, std::string_view& value) noexcept override;
        DbError getDoubleByIndex(int index, double& value) noexcept override;
        DbError getBlobByIndex(int index, Blob& value) noexcept override;

        SqliteStatement(sqlite3_stmt *stmt) noexcept;
    };

    class SqliteConnection final : public detail::IConnection {
        sqlite3 *mConnection = nullptr;

        sqlite3_stmt *mBeginStmt = nullptr;
        sqlite3_stmt *mCommitStmt = nullptr;
        sqlite3_stmt *mRollbackStmt = nullptr;

        DbError prepare(std::string_view sql, sqlite3_stmt **stmt) noexcept;

        DbError close() noexcept override;

        DbError prepare(std::string_view sql, detail::IStatement **statement) noexcept override;

        DbError begin() noexcept override;

        DbError commit() noexcept override;

        DbError rollback() noexcept override;

        DbError setupInsert(const dao::TableInfo& table, std::string& sql) noexcept override;
        DbError setupInsertOrUpdate(const dao::TableInfo& table, std::string& sql) noexcept override;
        DbError setupInsertReturningPrimaryKey(const dao::TableInfo& table, std::string& sql) noexcept override;

        DbError setupUpdate(const dao::TableInfo& table, std::string& sql) noexcept override;

        DbError setupSelect(const dao::TableInfo& table, std::string& sql) noexcept override;

        DbError createTable(const dao::TableInfo& table) noexcept override;

        DbError tableExists(std::string_view table, bool& exists) noexcept override;

        DbError clientVersion(Version& version) const noexcept override;
        DbError serverVersion(Version& version) const noexcept override;

    public:
        SqliteConnection(sqlite3 *connection) noexcept;
    };
}
