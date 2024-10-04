#include "stdafx.hpp"

#include "drivers/common.hpp"

#include "db/core.hpp"
#include "db/error.hpp"
#include "db/connection.hpp"

namespace dao = sm::dao;
namespace db = sm::db;

using namespace sm::db;

///
/// resource cleanup
///

void detail::destroyEnvironment(detail::IEnvironment *impl) noexcept {
    if (impl->close())
        delete impl;
}

void detail::destroyStatement(detail::IStatement *impl) noexcept {
    if (DbError err = impl->finalize())
        CT_NEVER("Failed to close statement: %s (%d)", err.what(), err.code());

    delete impl;
}

void detail::destroyConnection(detail::IConnection *impl) noexcept {
    if (DbError err = impl->close())
        CT_NEVER("Failed to close connection: %s (%d)", err.what(), err.code());

    delete impl;
}

///
/// connection
///

DbResult<bool> Connection::tableExists(std::string_view name) noexcept {
    std::string sql = mImpl->setupTableExists();

    PreparedStatement stmt = TRY_RESULT(tryPrepareQuery(sql));
    stmt.bind("name").to(name);
    ResultSet results = TRY_RESULT(stmt.start());

    return TRY_RESULT(results.get<int>(0)) > 0;
}

DbResult<Version> Connection::clientVersion() const noexcept {
    Version version;
    if (DbError error = mImpl->clientVersion(version))
        return std::unexpected(error);

    return version;
}

DbResult<Version> Connection::serverVersion() const noexcept {
    Version version;
    if (DbError error = mImpl->serverVersion(version))
        return std::unexpected(error);

    return version;
}

DbResult<PreparedStatement> Connection::tryPrepareStatement(std::string_view sql, StatementType type) noexcept {
    detail::IStatement *statement = nullptr;
    if (DbError error = mImpl->prepare(sql, &statement))
        return std::unexpected(error);

    return PreparedStatement{statement, this, type};
}

DbResult<PreparedStatement> Connection::tryPrepareQuery(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eQuery);
}

DbResult<PreparedStatement> Connection::tryPrepareUpdate(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eModify);
}

DbResult<PreparedStatement> Connection::tryPrepareDefine(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eDefine);
}

DbResult<PreparedStatement> Connection::tryPrepareControl(std::string_view sql) noexcept {
    return tryPrepareStatement(sql, StatementType::eControl);
}

PreparedStatement Connection::prepareInsertImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupInsert(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareInsertOrUpdateImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupInsertOrUpdate(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table) noexcept(false) {
    CTASSERTF(table.hasPrimaryKey(), "Table `%s` has no primary key", table.name.data());
    std::string sql = mImpl->setupInsertReturningPrimaryKey(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareTruncateImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupTruncate(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareUpdateAllImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupUpdate(table);
    return prepareUpdate(sql);
}

PreparedStatement Connection::prepareSelectAllImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = mImpl->setupSelect(table);
    return prepareQuery(sql);
}

PreparedStatement Connection::prepareDropImpl(const dao::TableInfo& table) noexcept(false) {
    std::string sql = fmt::format("DROP TABLE {}", table.name);
    return prepareUpdate(sql);
}

DbError Connection::tryCreateTable(const dao::TableInfo& table) noexcept {
    if (tableExists(table.name).value_or(false))
        return DbError::ok();

    std::string sql = mImpl->setupCreateTable(table);
    if (DbError error = tryUpdateSql(sql))
        return error;

    if (table.isSingleton()) {
        std::string sql = mImpl->setupSingletonTrigger(table);
        if (DbError error = tryUpdateSql(sql))
            return error;
    }

    return DbError::ok();
}

DbResult<ResultSet> Connection::trySelectSql(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(tryPrepareQuery(sql));

    return stmt.start();
}

DbError Connection::tryUpdateSql(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_UNWRAP(tryPrepareUpdate(sql));

    return stmt.execute();
}

DbError Connection::begin() noexcept {
    return mImpl->begin();
}

DbError Connection::commit() noexcept {
    return mImpl->commit();
}

DbError Connection::rollback() noexcept {
    return mImpl->rollback();
}

///
/// environment
///

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
        case DbType::eDB2: return detail::db2(&env);
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
