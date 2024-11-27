#include "stdafx.hpp"

#include "sqlite/sqlite.hpp"
#include "sqlite/connection.hpp"
#include "sqlite/statement.hpp"

using namespace sm::db;
using namespace sm::db::sqlite;

static Version getSqliteVersion() {
    const char *name = sqlite3_libversion();
    int num = sqlite3_libversion_number();

    int major = (num >> 16) & 0xFF;
    int minor = (num >> 8) & 0xFF;
    int patch = num & 0xFF;

    return Version{name, major, minor, patch};
}

static detail::ConnectionInfo buildConnectionInfo() {
    Version client = getSqliteVersion();
    Version server = getSqliteVersion();

    return detail::ConnectionInfo {
        .clientVersion = client,
        .serverVersion = server,
        .boolType = DataType::eInteger,
        .dateTimeType = DataType::eInteger,
        .hasCommentOn = false,
        .hasNamedParams = true,
        .hasUsers = false,
    };
}

void CloseDb::operator()(sqlite3 *db) noexcept {
    int err = sqlite3_close(db);
    if (err != SQLITE_OK) {
        LOG_ERROR(DbLog, "Failed to close SQLite connection: {} ({})", sqlite3_errstr(err), err);
    }
}

DbError SqliteConnection::getConnectionError(int err) const noexcept {
    return sqlite::getError(err, mConnection.get());
}

SqliteStatement SqliteConnection::newStatement(std::string_view sql) noexcept(false) {
    sqlite3_stmt *stmt = nullptr;
    if (int err = sqlite3_prepare_v2(mConnection.get(), sql.data(), sql.size(), &stmt, nullptr))
        throw DbException{getConnectionError(err)};

    return stmt;
}

detail::IStatement *SqliteConnection::prepare(std::string_view sql) noexcept(false) {
    return new SqliteStatement{newStatement(sql)};
}

DbError SqliteConnection::begin() noexcept {
    return mBeginStmt.execute();
}

DbError SqliteConnection::commit() noexcept {
    return mCommitStmt.execute();
}

DbError SqliteConnection::rollback() noexcept {
    return mRollbackStmt.execute();
}

std::string SqliteConnection::setupInsert(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupInsert(table);
}

std::string SqliteConnection::setupInsertOrUpdate(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupInsertOrUpdate(table);
}

std::string SqliteConnection::setupInsertReturningPrimaryKey(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupInsertReturningPrimaryKey(table);
}

std::string SqliteConnection::setupTruncate(const dao::TableInfo& table) noexcept(false) {
    return fmt::format("DELETE FROM {};", table.name);
}

std::string SqliteConnection::setupUpdate(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupUpdate(table);
}

std::string SqliteConnection::setupSelect(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupSelect(table);
}

std::string SqliteConnection::setupSelectByPrimaryKey(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupSelectByPrimaryKey(table);
}

std::string SqliteConnection::setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupCreateSingletonTrigger(table.name);
}

std::string SqliteConnection::setupTableExists() noexcept(false) {
    return "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=:name";
}

std::string SqliteConnection::setupCreateTable(const dao::TableInfo& table) noexcept(false) {
    return sqlite::setupCreateTable(table);
}

static bool isBlankString(const char *text) noexcept {
    while (*text) {
        if (!isspace(*text)) {
            return false;
        }

        text += 1;
    }

    return true;
}

static void isBlankStringImpl(sqlite3_context *ctx, int argc, sqlite3_value **argv) {
    if (argc != 1)
        return;

    const char *text = (const char*)sqlite3_value_text(argv[0]);
    if (text == nullptr)
        return;

    sqlite3_result_int(ctx, isBlankString(text));
}

SqliteConnection::SqliteConnection(Sqlite3Handle connection)
    : detail::IConnection(buildConnectionInfo())
    , mConnection(std::move(connection))
    , mBeginStmt(newStatement("BEGIN;"))
    , mCommitStmt(newStatement("COMMIT;"))
    , mRollbackStmt(newStatement("ROLLBACK;"))
{
    [[maybe_unused]] int err = sqlite3_create_function(mConnection.get(), "IS_BLANK_STRING", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, isBlankStringImpl, nullptr, nullptr);
    CTASSERTF(err == SQLITE_OK, "Failed to create IS_BLANK_STRING function: %s (%d)", sqlite3_errmsg(mConnection.get()), err);
}
