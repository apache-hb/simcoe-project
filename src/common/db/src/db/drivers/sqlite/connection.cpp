#include "stdafx.hpp"

#include "sqlite.hpp"

#include "core/defer.hpp"

using namespace sm::db;

namespace sqlite = sm::db::detail::sqlite;
using SqliteConnection = sqlite::SqliteConnection;

DbError SqliteConnection::prepare(std::string_view sql, sqlite3_stmt **stmt) noexcept {
    if (int err = sqlite3_prepare_v2(mConnection, sql.data(), sql.size(), stmt, nullptr))
        return getError(err, mConnection);

    return DbError::ok();
}

DbError SqliteConnection::close() noexcept {
    sqlite3_finalize(mBeginStmt);
    sqlite3_finalize(mCommitStmt);
    sqlite3_finalize(mRollbackStmt);

    int err = sqlite3_close(mConnection);
    if (err != SQLITE_OK)
        return getError(err, mConnection);

    mConnection = nullptr;
    return DbError::ok();
}

DbError SqliteConnection::prepare(std::string_view sql, detail::IStatement **statement) noexcept {
    sqlite3_stmt *stmt = nullptr;
    if (DbError err = prepare(sql, &stmt))
        return err;

    *statement = new SqliteStatement{stmt};
    return DbError::ok();
}

DbError SqliteConnection::begin() noexcept {
    int err = execStatement(mBeginStmt);
    return getError(err, mConnection);
}

DbError SqliteConnection::commit() noexcept {
    int err = execStatement(mCommitStmt);
    return getError(err, mConnection);
}

DbError SqliteConnection::rollback() noexcept {
    int err = execStatement(mRollbackStmt);
    return getError(err, mConnection);
}

DbError SqliteConnection::setupInsert(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = sqlite::setupInsert(table);
    return DbError::ok();
}

DbError SqliteConnection::setupInsertOrUpdate(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = sqlite::setupInsertOrUpdate(table);
    return DbError::ok();
}

DbError SqliteConnection::setupInsertReturningPrimaryKey(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = sqlite::setupInsertReturningPrimaryKey(table);
    return DbError::ok();
}

DbError SqliteConnection::createTable(const dao::TableInfo& table) noexcept {
    auto sql = setupCreateTable(table);
    if (int err = sqlite3_exec(mConnection, sql.c_str(), nullptr, nullptr, nullptr))
        return getError(err, mConnection);

    return DbError::ok();
}

DbError SqliteConnection::tableExists(std::string_view table, bool& exists) noexcept {
    sqlite3_stmt *stmt = nullptr;
    if (DbError err = prepare("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=:name", &stmt))
        return err;

    SqliteStatement ps{stmt};
    defer { (void)ps.close(); };

    if (DbError err = ps.bindStringByName("name", table))
        return err;

    int64 count = 0;
    while (ps.next().isSuccess()) {
        if (DbError err = ps.getIntByIndex(0, count))
            return err;
    }

    exists = count > 0;

    return DbError::ok();
}

static Version getSqliteVersion() noexcept {
    const char *name = sqlite3_libversion();
    int num = sqlite3_libversion_number();

    int major = (num >> 16) & 0xFF;
    int minor = (num >> 8) & 0xFF;
    int patch = num & 0xFF;

    return Version{name, major, minor, patch};
}

DbError SqliteConnection::clientVersion(Version& version) const noexcept {
    version = getSqliteVersion();
    return DbError::ok();
}

DbError SqliteConnection::serverVersion(Version& version) const noexcept {
    version = getSqliteVersion();
    return DbError::ok();
}

static bool isBlankString(const char *text) noexcept {
    while (*text)
        if (!isspace(*text))
            return false;

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

SqliteConnection::SqliteConnection(sqlite3 *connection) noexcept
    : mConnection(connection)
{
    if (int err = sqlite3_prepare_v2(mConnection, "BEGIN;", -1, &mBeginStmt, nullptr))
        CT_NEVER("Failed to prepare BEGIN statement: %s (%d)", sqlite3_errmsg(connection), err);

    if (int err = sqlite3_prepare_v2(mConnection, "COMMIT;", -1, &mCommitStmt, nullptr))
        CT_NEVER("Failed to prepare COMMIT statement: %s (%d)", sqlite3_errmsg(connection), err);

    if (int err = sqlite3_prepare_v2(mConnection, "ROLLBACK;", -1, &mRollbackStmt, nullptr))
        CT_NEVER("Failed to prepare ROLLBACK statement: %s (%d)", sqlite3_errmsg(connection), err);

    int err = sqlite3_create_function(mConnection, "IS_BLANK_STRING", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, isBlankStringImpl, nullptr, nullptr);
    if (err != SQLITE_OK)
        CT_NEVER("Failed to create IS_BLANK_STRING function: %s (%d)", sqlite3_errmsg(connection), err);
}

class SqliteEnvironment final : public detail::IEnvironment {
    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override {
        std::string dbPath = std::string{config.host};
        sqlite3 *db = nullptr;
        if (int err = sqlite3_open(dbPath.c_str(), &db))
            return sqlite::getError(err);

        sqlite3_trace_v2(db, SQLITE_TRACE_STMT, [](unsigned flags, void *ctx, void *p, void *x) {
            sqlite3_stmt *stmt = (sqlite3_stmt*)p;
            const char *sql = sqlite3_expanded_sql(stmt);
            fmt::println(stderr, "SQL: {}", sql);
            return 0;
        }, nullptr);

        *connection = new SqliteConnection{db};
        return DbError::ok();
    }

    bool close() noexcept override {
        return false;
    }

public:
    SqliteEnvironment() noexcept {
        int err = sqlite3_config(SQLITE_CONFIG_LOG, +[](void *ctx, int err, const char *msg) {
            fmt::println(stderr, "SQLite: {}", msg);
        }, nullptr);
        CTASSERTF(err == SQLITE_OK, "Failed to configure SQLite logging: %d", err);
    }
};

DbError detail::getSqliteEnv(detail::IEnvironment **env) noexcept {
    static SqliteEnvironment sqlite;
    *env = &sqlite;
    return DbError::ok();
}
