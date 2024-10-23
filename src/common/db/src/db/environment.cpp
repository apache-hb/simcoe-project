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

DbResult<Environment> Environment::tryCreate(DbType type, const EnvConfig& config) noexcept {
    detail::IEnvironment *env = nullptr;
    DbError error = [&] {
        switch (type) {
        case DbType::eSqlite3: return detail::getSqliteEnv(&env, config);

#if SMC_DB_HAS_POSTGRES
        case DbType::ePostgreSQL: return detail::getPostgresEnv(&env);
#endif

#if SMC_DB_HAS_MYSQL
        case DbType::eMySQL: return detail::mysql(&env);
#endif

#if SMC_DB_HAS_ORCL
        case DbType::eOracleDB: return detail::getOracleEnv(&env, config);
#endif

#if SMC_DB_HAS_MSSQL
        case DbType::eMSSQL: return detail::mssql(&env);
#endif

#if SMC_DB_HAS_DB2
        case DbType::eDB2: return detail::getDb2Env(&env);
#endif

#if SMC_DB_HAS_ODBC
        case DbType::eODBC: return detail::odbc(&env);
#endif

        default: return DbError::unsupported("environment");
        }
    }();

    if (error)
        return std::unexpected(error);

    return Environment{env};
}

DbResult<Connection> Environment::tryConnect(const ConnectionConfig& config) noexcept {
    detail::IConnection *connection = nullptr;

    LOG_INFO(DbLog, "Connecting to database: {}:{}/{} as role `{}`", config.host, config.port, config.database, config.user);
    if (DbError error = mImpl->connect(config, &connection))
        return std::unexpected(error);

    return Connection{connection, config};
}
