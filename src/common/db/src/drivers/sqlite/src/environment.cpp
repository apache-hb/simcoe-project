#include "stdafx.hpp"

#include "sqlite/sqlite.hpp"
#include "sqlite/environment.hpp"
#include "sqlite/connection.hpp"

#include "core/memory.hpp"
#include "core/units.hpp"
#include "base/defer.hpp"

#include <tlsf.h>

using namespace sm::db;
using namespace sm::db::sqlite;

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

static void setConfigMemory() {
    int err = sqlite3_config(SQLITE_CONFIG_MALLOC, &kMemoryMethods);
    if (err != SQLITE_OK) {
        LOG_WARN(DbLog, "Failed to set SQLite memory: {} ({})", sqlite3_errstr(err), err);
    }
}

static void setConfigLog() {
    auto fn = +[](void *ctx, int err, const char *msg) {
        LOG_INFO(DbLog, "sqlite: {}", msg);
    };
    int err = sqlite3_config(SQLITE_CONFIG_LOG, fn, nullptr);

    if (err != SQLITE_OK) {
        LOG_WARN(DbLog, "Failed to set SQLite log: {} ({})", sqlite3_errstr(err), err);
    }
}

DbError SqliteEnvironment::execute(sqlite3 *db, const std::string& sql) noexcept {
    char *message = nullptr;
    defer { if (message != nullptr) sqlite3_free(message); };
    int err = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &message);

    if (err != SQLITE_OK) {
        const char *msg = message ?: sqlite3_errmsg(db);
        return sqlite::getError(err, db, msg);
    }

    return DbError::ok();
}

void SqliteEnvironment::pragma(sqlite3 *db, std::string_view option, std::string_view value) noexcept(false) {
    execute(db, fmt::format("PRAGMA {} = {}", option, value)).throwIfFailed();
}

bool SqliteEnvironment::close() noexcept {
    return false;
}

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

detail::IConnection *SqliteEnvironment::connect(const ConnectionConfig& config) noexcept(false) {
    sqlite::Sqlite3Handle db;
    if (int err = sqlite3_open(config.host.c_str(), &db)) {
        throw DbConnectionException{sqlite::getError(err), config};
    }

    if (config.journalMode != JournalMode::eDefault) {
        pragma(db.get(), "journal_mode", kJournalMode[std::to_underlying(config.journalMode)]);
    }

    if (config.synchronous != Synchronous::eDefault) {
        pragma(db.get(), "synchronous", kSynchronous[std::to_underlying(config.synchronous)]);
    }

    if (config.lockingMode != LockingMode::eDefault) {
        pragma(db.get(), "locking_mode", kLockingMode[std::to_underlying(config.lockingMode)]);
    }

    return new SqliteConnection{std::move(db)};
}

SqliteEnvironment::SqliteEnvironment(const EnvConfig& config) noexcept {
    if (config.logQueries)
        setConfigLog();

    setConfigMemory();
}

detail::IEnvironment *detail::newSqliteEnvironment(const EnvConfig& config) {
    static SqliteEnvironment sEnvironment{config};
    return &sEnvironment;
}
