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
        CT_NEVER("Failed to close statement: %s (%d)", err.message().data(), err.code());

    delete impl;
}

void detail::destroyConnection(detail::IConnection *impl) noexcept {
    if (DbError err = impl->close())
        CT_NEVER("Failed to close connection: %s (%d)", err.message().data(), err.code());

    delete impl;
}

///
/// result set
///

DbError ResultSet::next() noexcept {
    return mImpl->next();
}

DbError ResultSet::execute() noexcept {
    return mImpl->execute();
}

int ResultSet::getColumnCount() const noexcept {
    return mImpl->getColumnCount();
}

DbResult<ColumnInfo> ResultSet::getColumnInfo(int index) const noexcept {
    ColumnInfo column;
    if (DbError error = mImpl->getColumnInfo(index, column))
        return std::unexpected(error);

    return column;
}

DbResult<double> ResultSet::getDouble(int index) noexcept {
    double value = 0.0;
    if (DbError error = mImpl->getDoubleByIndex(index, value))
        return error;

    return value;
}

DbResult<sm::int64> ResultSet::getInt(int index) noexcept {
    int64 value = 0;
    if (DbError error = mImpl->getIntByIndex(index, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(int index) noexcept {
    bool value = false;
    if (DbError error = mImpl->getBooleanByIndex(index, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(int index) noexcept {
    std::string_view value;
    if (DbError error = mImpl->getStringByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(int index) noexcept {
    Blob value;
    if (DbError error = mImpl->getBlobByIndex(index, value))
        return std::unexpected(error);

    return value;
}

DbResult<double> ResultSet::getDouble(std::string_view column) noexcept {
    double value = 0.0;
    if (DbError error = mImpl->getDoubleByName(column, value))
        return error;

    return value;
}

DbResult<sm::int64> ResultSet::getInt(std::string_view column) noexcept {
    int64 value = 0;
    if (DbError error = mImpl->getIntByName(column, value))
        return error;

    return value;
}

DbResult<bool> ResultSet::getBool(std::string_view column) noexcept {
    bool value = false;
    if (DbError error = mImpl->getBooleanByName(column, value))
        return error;

    return value;
}

DbResult<std::string_view> ResultSet::getString(std::string_view column) noexcept {
    std::string_view value;
    if (DbError error = mImpl->getStringByName(column, value))
        return std::unexpected(error);

    return value;
}

DbResult<Blob> ResultSet::getBlob(std::string_view column) noexcept {
    Blob value;
    if (DbError error = mImpl->getBlobByName(column, value))
        return std::unexpected(error);

    return value;
}

///
/// named bindpoint
///

void BindPoint::toInt(int64 value) noexcept(false) {
    mImpl->bindIntByName(mName, value).throwIfFailed();
}

void BindPoint::toBool(bool value) noexcept(false) {
    mImpl->bindBooleanByName(mName, value).throwIfFailed();
}

void BindPoint::toString(std::string_view value) noexcept(false) {
    mImpl->bindStringByName(mName, value).throwIfFailed();
}

void BindPoint::toDouble(double value) noexcept(false) {
    mImpl->bindDoubleByName(mName, value).throwIfFailed();
}

void BindPoint::toBlob(Blob value) noexcept(false) {
    mImpl->bindBlobByName(mName, value).throwIfFailed();
}

void BindPoint::toNull() noexcept(false) {
    mImpl->bindNullByName(mName).throwIfFailed();
}

///
/// statement
///

BindPoint PreparedStatement::bind(std::string_view name) noexcept {
    return BindPoint{mImpl.get(), name};
}

DbError PreparedStatement::close() noexcept {
    return mImpl->finalize();
}

DbResult<ResultSet> PreparedStatement::select() noexcept {
    if (DbError error = mImpl->select())
        return std::unexpected(error);

    return ResultSet{mImpl};
}

DbResult<ResultSet> PreparedStatement::update() noexcept {
    if (DbError error = mImpl->update(mConnection->autoCommit()))
        return std::unexpected(error);

    return ResultSet{mImpl};
}

DbResult<ResultSet> PreparedStatement::start() noexcept {
    if (DbError error = mImpl->start(mConnection->autoCommit(), type()))
        return std::unexpected(error);

    return ResultSet{mImpl};
}

DbError PreparedStatement::execute() noexcept {
    auto result = TRY_UNWRAP(start());
    return result.execute();
}

///
/// connection
///

DbResult<bool> Connection::tableExists(std::string_view name) noexcept {
    bool exists = false;
    if (DbError error = mImpl->tableExists(name, exists))
        return error;

    return exists;
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

DbResult<PreparedStatement> Connection::sqlPrepare(std::string_view sql, StatementType type) noexcept {
    detail::IStatement *statement = nullptr;
    if (DbError error = mImpl->prepare(sql, &statement))
        return std::unexpected(error);

    return PreparedStatement{statement, this, type};
}

DbResult<PreparedStatement> Connection::dqlPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eQuery);
}

DbResult<PreparedStatement> Connection::dmlPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eModify);
}

DbResult<PreparedStatement> Connection::ddlPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eDefine);
}

DbResult<PreparedStatement> Connection::dclPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eControl);
}

static void bindIndex(PreparedStatement& stmt, const dao::TableInfo& info, size_t index, bool returning, const void *data) noexcept {
    const auto& column = info.columns[index];
    if (returning && info.primaryKey == index)
        return;

    auto binding = stmt.bind(column.name);
    const void *field = static_cast<const char*>(data) + column.offset;

    switch (column.type) {
    case dao::ColumnType::eInt:
        binding.toInt(*reinterpret_cast<const int32_t*>(field));
        break;
    case dao::ColumnType::eUint:
        binding.toInt(*reinterpret_cast<const uint32_t*>(field));
        break;
    case dao::ColumnType::eLong:
        binding.toInt(*reinterpret_cast<const int64_t*>(field));
        break;
    case dao::ColumnType::eUlong:
        binding.toInt(*reinterpret_cast<const uint64_t*>(field));
        break;
    case dao::ColumnType::eBool:
        binding.toBool(*reinterpret_cast<const bool*>(field));
        break;
    case dao::ColumnType::eString:
        binding.toString(*reinterpret_cast<const std::string*>(field));
        break;
    case dao::ColumnType::eFloat:
        binding.toDouble(*reinterpret_cast<const float*>(field));
        break;
    case dao::ColumnType::eDouble:
        binding.toDouble(*reinterpret_cast<const double*>(field));
        break;
    default:
        CT_NEVER("Unsupported column type");
    }
}

void PreparedStatement::bindRowData(const dao::TableInfo& info, bool returning, const void *data) noexcept {
    for (size_t i = 0; i < info.columns.size(); i++)
        bindIndex(*this, info, i, returning, data);
}

DbResult<PreparedStatement> Connection::prepareInsertImpl(const dao::TableInfo& table) noexcept {
    std::string sql;
    if (DbError error = mImpl->setupInsert(table, sql))
        return std::unexpected(error);

    return TRY_RESULT(dmlPrepare(sql));
}

DbResult<PreparedStatement> Connection::prepareInsertOrUpdateImpl(const dao::TableInfo& table) noexcept {
    std::string sql;
    if (DbError error = mImpl->setupInsertOrUpdate(table, sql))
        return std::unexpected(error);

    return TRY_RESULT(dmlPrepare(sql));
}

DbResult<PreparedStatement> Connection::prepareInsertReturningPrimaryKeyImpl(const dao::TableInfo& table) noexcept {
    std::string sql;
    if (DbError error = mImpl->setupInsertReturningPrimaryKey(table, sql))
        return std::unexpected(error);

    return TRY_RESULT(dmlPrepare(sql));
}

DbResult<PreparedStatement> Connection::prepareSelectImpl(const dao::TableInfo& table) noexcept {
    std::string sql;
    if (DbError error = mImpl->setupSelect(table, sql))
        return std::unexpected(error);

    return TRY_RESULT(dqlPrepare(sql));
}

DbError Connection::tryCreateTable(const dao::TableInfo& table) noexcept {
    if (tableExists(table.name).value_or(false))
        return DbError::ok();

    return mImpl->createTable(table);
}

DbResult<ResultSet> Connection::select(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(dqlPrepare(sql));

    return stmt.select();
}

DbResult<ResultSet> Connection::update(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(dmlPrepare(sql));

    return stmt.update();
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
#define DB_TYPE(id, str, enabled) case DbType::id: return enabled;
#include "db/orm.inc"
    }
}

std::string_view db::toString(DbType type) noexcept {
    switch (type) {
#define DB_TYPE(id, str, enabled) case DbType::id: return str;
#include "db/orm.inc"
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
        case DbType::eOracleDB: return detail::getOracleEnv(&env);
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

    gLog.info("Connecting to database: {}:{}/{} as role `{}`", config.host, config.port, config.database, config.user);
    if (DbError error = mImpl->connect(config, &connection))
        return std::unexpected(error);

    return Connection{connection, config};
}
