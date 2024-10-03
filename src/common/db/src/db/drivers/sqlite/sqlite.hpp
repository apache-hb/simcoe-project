#pragma once

#include "drivers/common.hpp"

namespace sm::db::detail::sqlite {
    std::string setupTableExists() noexcept;
    std::string setupCreateTable(const dao::TableInfo& info) noexcept;
    std::string setupCreateSingletonTrigger(std::string_view name) noexcept;
    std::string setupInsert(const dao::TableInfo& info) noexcept;
    std::string setupInsertOrUpdate(const dao::TableInfo& info) noexcept;
    std::string setupInsertReturningPrimaryKey(const dao::TableInfo& info) noexcept;
    std::string setupTruncate(std::string_view name) noexcept;
    std::string setupUpdate(const dao::TableInfo& info) noexcept;
    std::string setupSelect(const dao::TableInfo& info) noexcept;

    DbError getError(int err) noexcept;
    DbError getError(int err, sqlite3 *db, const char *message = nullptr) noexcept;

    int execStatement(sqlite3_stmt *stmt) noexcept;

    constexpr auto kCloseDb = [](sqlite3 *db) noexcept {
        int err = sqlite3_close(db);
        if (err != SQLITE_OK) {
            LOG_WARN(DbLog, "Failed to close SQLite connection: {} ({})", sqlite3_errstr(err), err);
        }
    };

    using Sqlite3Handle = sm::UniquePtr<sqlite3, decltype(kCloseDb)>;

    class SqliteStatement final : public detail::IStatement {
        sqlite3_stmt *mStatement = nullptr;
        int mStatus = SQLITE_OK;

        DbError getStmtError(int err) const noexcept;
        bool hasDataReady() const noexcept override { return mStatus == SQLITE_ROW; }

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
        DbError bindDateTimeByIndex(int index, DateTime value) noexcept override;
        DbError bindNullByIndex(int index) noexcept override;

        int getColumnCount() const noexcept override;
        DbError getColumnIndex(std::string_view name, int& index) const noexcept override;

        DbError getColumnInfo(int index, ColumnInfo& info) const noexcept override;
        DbError getColumnInfo(std::string_view name, ColumnInfo& info) const noexcept override;

        DbError getIntByIndex(int index, int64& value) noexcept override;
        DbError getBooleanByIndex(int index, bool& value) noexcept override;
        DbError getStringByIndex(int index, std::string_view& value) noexcept override;
        DbError getDoubleByIndex(int index, double& value) noexcept override;
        DbError getDateTimeByIndex(int index, DateTime& value) noexcept override;
        DbError getBlobByIndex(int index, Blob& value) noexcept override;

        DbError isNullByIndex(int index, bool& value) noexcept override;
        DbError isNullByName(std::string_view column, bool& value) noexcept override;

        SqliteStatement(sqlite3_stmt *stmt) noexcept;
    };

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

        DbError clientVersion(Version& version) const noexcept override;
        DbError serverVersion(Version& version) const noexcept override;

        DataType boolEquivalentType() const noexcept override { return DataType::eInteger; }
        DataType dateTimeEquivalentType() const noexcept override { return DataType::eInteger; }

    public:
        SqliteConnection(Sqlite3Handle connection) noexcept;
    };
}
