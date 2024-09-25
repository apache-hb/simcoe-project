#include "stdafx.hpp"

#include "sqlite.hpp"

#include "core/memory.hpp"
#include "core/units.hpp"
#include "core/defer.hpp"

#include <tlsf.h>

using namespace sm::db;

namespace sqlite = sm::db::detail::sqlite;
using SqliteConnection = sqlite::SqliteConnection;

DbError SqliteConnection::getConnectionError(int err) const noexcept {
    return sqlite::getError(err, mConnection.get());
}

DbError SqliteConnection::prepare(std::string_view sql, sqlite3_stmt **stmt) noexcept {
    if (int err = sqlite3_prepare_v2(mConnection.get(), sql.data(), sql.size(), stmt, nullptr))
        return getError(err, mConnection.get());

    return DbError::ok();
}

DbError SqliteConnection::close() noexcept {
    sqlite3_finalize(mBeginStmt);
    sqlite3_finalize(mCommitStmt);
    sqlite3_finalize(mRollbackStmt);

    int err = sqlite3_close(mConnection.release());
    if (err != SQLITE_OK)
        return getConnectionError(err);

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
    return getConnectionError(err);
}

DbError SqliteConnection::commit() noexcept {
    int err = execStatement(mCommitStmt);
    return getConnectionError(err);
}

DbError SqliteConnection::rollback() noexcept {
    int err = execStatement(mRollbackStmt);
    return getConnectionError(err);
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

DbError SqliteConnection::setupUpdate(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = sqlite::setupUpdate(table);
    return DbError::ok();
}

DbError SqliteConnection::setupSelect(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = sqlite::setupSelect(table);
    return DbError::ok();
}

DbError SqliteConnection::setupSingletonTrigger(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = sqlite::setupCreateSingletonTrigger(table.name);
    return DbError::ok();
}

DbError SqliteConnection::setupTableExists(std::string& sql) noexcept {
    sql = sqlite::setupTableExists();
    return DbError::ok();
}

DbError SqliteConnection::createTable(const dao::TableInfo& table) noexcept {
    auto sql = setupCreateTable(table);
    if (int err = sqlite3_exec(mConnection.get(), sql.c_str(), nullptr, nullptr, nullptr))
        return getError(err, mConnection.get());

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

SqliteConnection::SqliteConnection(Sqlite3Handle connection) noexcept
    : mConnection(std::move(connection))
{
    prepareAlways(mConnection.get(), "BEGIN;", &mBeginStmt);
    prepareAlways(mConnection.get(), "COMMIT;", &mCommitStmt);
    prepareAlways(mConnection.get(), "ROLLBACK;", &mRollbackStmt);

    [[maybe_unused]] int err = sqlite3_create_function(mConnection.get(), "IS_BLANK_STRING", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, nullptr, isBlankStringImpl, nullptr, nullptr);
    CTASSERTF(err == SQLITE_OK, "Failed to create IS_BLANK_STRING function: %s (%d)", sqlite3_errmsg(mConnection.get()), err);
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

static constexpr std::string_view kJournalMode[(int)JournalMode::eCount] = {
    [(int)JournalMode::eDelete] = "DELETE",
    [(int)JournalMode::eTruncate] = "TRUNCATE",
    [(int)JournalMode::ePersist] = "PERSIST",
    [(int)JournalMode::eMemory] = "MEMORY",
    [(int)JournalMode::eWal] = "WAL",
    [(int)JournalMode::eOff] = "OFF",
};

static constexpr std::string_view kSynchronous[(int)Synchronous::eCount] = {
    [(int)Synchronous::eExtra] = "EXTRA",
    [(int)Synchronous::eFull] = "FULL",
    [(int)Synchronous::eNormal] = "NORMAL",
    [(int)Synchronous::eOff] = "OFF",
};

static constexpr std::string_view kLockingMode[(int)LockingMode::eCount] = {
    [(int)LockingMode::eRelaxed] = "NORMAL",
    [(int)LockingMode::eExclusive] = "EXCLUSIVE",
};

class SqliteEnvironment final : public detail::IEnvironment {
    DbError execute(sqlite3 *db, const std::string& sql) noexcept {
        char *message = nullptr;
        defer { if (message != nullptr) sqlite3_free(message); };
        int err = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &message);

        if (err != SQLITE_OK) {
            const char *msg = message != nullptr ? message : sqlite3_errmsg(db);
            return sqlite::getError(err, db, msg);
        }

        return DbError::ok();
    }

    DbError pragma(sqlite3 *db, std::string_view option, std::string_view value) noexcept {
        std::string sql = fmt::format("PRAGMA {} = {}", option, value);
        return execute(db, sql);
    }

    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override {
        std::string dbPath = std::string{config.host};
        sqlite::Sqlite3Handle db;
        if (int err = sqlite3_open(dbPath.c_str(), &db))
            return sqlite::getError(err);

        if (config.journalMode != JournalMode::eDefault) {
            if (DbError err = pragma(db.get(), "journal_mode", kJournalMode[(int)config.journalMode]))
                return err;
        }

        if (config.synchronous != Synchronous::eDefault) {
            if (DbError err = pragma(db.get(), "synchronous", kSynchronous[(int)config.synchronous]))
                return err;
        }

        if (config.lockingMode != LockingMode::eDefault) {
            if (DbError err = pragma(db.get(), "locking_mode", kLockingMode[(int)config.lockingMode]))
                return err;
        }

        *connection = new SqliteConnection{std::move(db)};
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
