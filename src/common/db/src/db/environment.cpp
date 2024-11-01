#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/db.hpp"
#include "db/error.hpp"
#include "db/environment.hpp"

using namespace sm::db;

void detail::destroyEnvironment(detail::IEnvironment *impl) noexcept {
    if (impl->close())
        delete impl;
}

bool Environment::isSupported(DbType type) noexcept {
    switch (type) {
#define DB_TYPE(id, enabled) case DbType::id: return enabled;
#include "db/orm.inc"

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

static std::string formatConnectionConfig(const ConnectionConfig& config) {
    std::string_view role = config.user.empty() ? "anonymous" : std::string_view(config.user);
    std::string database = config.database.empty() ? "" : fmt::format("/{}", config.database);
    std::string port = (config.port == 0) ? "" : fmt::format(":{}", config.port);

    return fmt::format("{}{}{} as `{}`", config.host, port, database, role);
}

static void logConnectionAttempt(const ConnectionConfig& config) {
    std::string info = formatConnectionConfig(config);

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
    logConnectionAttempt(config);

    detail::IConnection *connection = mImpl->connect(config);
    return Connection{connection, config};
}
