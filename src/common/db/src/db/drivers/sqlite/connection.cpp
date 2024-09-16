#include "stdafx.hpp"

#include "sqlite.hpp"

#include "core/memory.hpp"
#include "core/units.hpp"
#include "core/defer.hpp"

#include <tlsf.h>

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
    defer { (void)ps.finalize(); };

    if (DbError err = ps.bindStringByName("name", table))
        return err;

    if (DbError err = ps.start(false, StatementType::eQuery))
        return err;

    int64 count = 0;
    if (DbError err = ps.getIntByIndex(0, count))
        return err;

    if (DbError err = ps.execute())
        return err;

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

static void prepareAlways(sqlite3 *connection, const char *sql, sqlite3_stmt **stmt) {
    [[maybe_unused]] int err = sqlite3_prepare_v2(connection, sql, -1, stmt, nullptr);
    CTASSERTF(err == SQLITE_OK, "Failed to prepare statement `%s`: %s (%d)", sql, sqlite3_errmsg(connection), err);
}

SqliteConnection::SqliteConnection(sqlite3 *connection) noexcept
    : mConnection(connection)
{
    prepareAlways(connection, "BEGIN;", &mBeginStmt);
    prepareAlways(connection, "COMMIT;", &mCommitStmt);
    prepareAlways(connection, "ROLLBACK;", &mRollbackStmt);

    [[maybe_unused]] int err = sqlite3_create_function(mConnection, "IS_BLANK_STRING", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, isBlankStringImpl, nullptr, nullptr);
    CTASSERTF(err == SQLITE_OK, "Failed to create IS_BLANK_STRING function: %s (%d)", sqlite3_errmsg(connection), err);
}

static void *gPoolMemory = nullptr;
static tlsf_t gPool = nullptr;

static void sqliteShutdown(void*) noexcept {
    tlsf_destroy(gPool);

    free(gPoolMemory);
}

static int sqliteInit(void*) noexcept {
    size_t size = sm::megabytes(32).asBytes();
    gPoolMemory = malloc(size);
    CTASSERT(gPoolMemory != nullptr);

    gPool = tlsf_create_with_pool(gPoolMemory, size);
    CTASSERT(gPool != nullptr);

    return SQLITE_OK;
}

static constexpr sqlite3_mem_methods kMemoryMethods = {
    .xMalloc = [](int size) { return tlsf_malloc(gPool, size); },
    .xFree = [](void *ptr) { tlsf_free(gPool, ptr); },
    .xRealloc = [](void *ptr, int size) { return tlsf_realloc(gPool, ptr, size); },
    .xSize = [](void *ptr) { return (int)tlsf_block_size(ptr); },
    .xRoundup = [](int size) { return sm::roundup<int>(size, tlsf_block_size_min()); },
    .xInit = sqliteInit,
    .xShutdown = sqliteShutdown,
};

class SqliteEnvironment final : public detail::IEnvironment {
    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override {
        std::string dbPath = std::string{config.host};
        sqlite3 *db = nullptr;
        if (int err = sqlite3_open(dbPath.c_str(), &db))
            return sqlite::getError(err);

        // TODO: expose these through config options
#if 0
        sqlite3_exec(db, "PRAGMA synchronous = OFF", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", nullptr, nullptr, nullptr);
        sqlite3_exec(db, "PRAGMA locking_mode = EXCLUSIVE", nullptr, nullptr, nullptr);
#endif

        *connection = new SqliteConnection{db};
        return DbError::ok();
    }

    bool close() noexcept override {
        return false;
    }

    static void setConfigLog() {
        int err = sqlite3_config(SQLITE_CONFIG_LOG, +[](void *ctx, int err, const char *msg) {
            fmt::println(stderr, "SQLite: {}", msg);
        }, nullptr);

        if (err != SQLITE_OK)
            fmt::println(stderr, "Failed to set SQLite log: %s (%d)", sqlite3_errstr(err), err);
    }

    static void setConfigMemory() {
        int err = sqlite3_config(SQLITE_CONFIG_MALLOC, &kMemoryMethods);
        if (err != SQLITE_OK)
            fmt::println(stderr, "Failed to set SQLite memory: {} ({})", sqlite3_errstr(err), err);
    }

public:
    SqliteEnvironment(const EnvConfig& config) noexcept {
        if (config.logQueries)
            setConfigLog();


        setConfigMemory();
    }
};

DbError detail::getSqliteEnv(detail::IEnvironment **env, const EnvConfig& config) noexcept {
    static SqliteEnvironment sqlite{config};
    *env = &sqlite;
    return DbError::ok();
}
