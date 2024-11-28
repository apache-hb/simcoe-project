#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/db.hpp"
#include "db/error.hpp"
#include "db/environment.hpp"

using namespace sm;
using namespace sm::db;

std::string_view db::toString(DbType it) noexcept {
    using enum DbType;
    switch (it) {
#define DB_TYPE(id, name, enabled) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string_view db::toString(DataType it) noexcept {
    using enum DataType;
    switch (it) {
#define DB_DATATYPE(id, name) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string_view db::toString(StatementType it) noexcept {
    using enum StatementType;
    switch (it) {
#define DB_STATEMENT(id, name) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string_view db::toString(JournalMode it) noexcept {
    using enum JournalMode;
    switch (it) {
#define DB_JOURNAL(id, name) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string_view db::toString(Synchronous it) noexcept {
    using enum Synchronous;
    switch (it) {
#define DB_SYNC(id, name) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string_view db::toString(LockingMode it) noexcept {
    using enum LockingMode;
    switch (it) {
#define DB_LOCKING(id, name) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string_view db::toString(AssumeRole it) noexcept {
    using enum AssumeRole;
    switch (it) {
#define DB_ROLE(id, name) case id: return name;
#include "db/db.inc"
    default: return "Unknown";
    }
}

std::string db::toString(const ConnectionConfig& config) {
    std::string_view role = config.user.empty() ? "anonymous" : std::string_view(config.user);
    std::string database = config.database.empty() ? "" : fmt::format("/{}", config.database);
    std::string port = (config.port == 0) ? "" : fmt::format(":{}", config.port);

    return fmt::format("{}{}{} as {}", config.host, port, database, role);
}

void detail::destroyEnvironment(detail::IEnvironment *impl) noexcept {
    if (impl->close())
        delete impl;
}

bool Environment::isSupported(DbType type) noexcept {
    switch (type) {
#define DB_TYPE(id, name, enabled) case DbType::id: return enabled;
#include "db/db.inc"

    default: return false;
    }
}

DbResult<Environment> Environment::tryCreate(DbType type, const EnvConfig& config) noexcept try {
    return Environment::create(type, config);
} catch (const DbException& ex) {
    return std::unexpected(ex.error());
}

Environment Environment::create(DbType type, const EnvConfig& config) {
    switch (type) {
    case DbType::eSqlite3:
        return detail::newSqliteEnvironment(config);

#if SMC_DB_HAS_ORCL
    case DbType::eOracleDB:
        return detail::newOracleEnvironment(config);
#endif

#if SMC_DB_HAS_DB2
    case DbType::eDB2:
        return detail::newDb2Environment();
#endif

    default:
        throw DbException{DbError::unsupported(type)};
    }
}

static void logConnectionAttempt(const ConnectionConfig& config) {
    std::string info = toString(config);

    if (config.timeout != kDefaultTimeout) {
        info += fmt::format(" (timeout: {})", config.timeout);
    }

    LOG_INFO(DbLog, "Connecting to database: {}", info);
}

DbResult<Connection> Environment::tryConnect(const ConnectionConfig& config) noexcept try {
    return connect(config);
} catch (const DbException& ex) {
    return std::unexpected(ex.error());
}

Connection Environment::connect(const ConnectionConfig& config) {
    if (config.host.empty()) {
        throw DbException{DbError::invalidHost("Host name is empty")};
    }

    logConnectionAttempt(config);

    detail::IConnection *connection = mImpl->connect(config);
    return Connection{connection, config};
}
