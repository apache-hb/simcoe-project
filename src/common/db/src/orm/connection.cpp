#include "orm/core.hpp"
#include "orm/error.hpp"
#include "stdafx.hpp"

#include "db/common.hpp"

#include "orm/connection.hpp"

using namespace sm;
using namespace sm::db;

///
/// resource cleanup
///

void detail::destroyEnvironment(detail::IEnvironment *impl) noexcept {
    if (impl->close())
        delete impl;
}

void detail::destroyStatement(detail::IStatement *impl) noexcept {
    if (DbError err = impl->close())
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

int ResultSet::getColumnCount() const noexcept {
    return mImpl->getColumnCount();
}

std::expected<ColumnInfo, DbError> ResultSet::getColumnInfo(int index) const noexcept {
    ColumnInfo column;
    if (DbError error = mImpl->getColumnInfo(index, column))
        return std::unexpected(error);

    return column;
}


double ResultSet::getDouble(int index) noexcept(false) {
    double value = 0.0;
    if (DbError error = mImpl->getDoubleByIndex(index, value))
        error.raise();

    return value;
}

int64 ResultSet::getInt(int index) noexcept(false) {
    int64 value = 0;
    if (DbError error = mImpl->getIntByIndex(index, value))
        error.raise();

    return value;
}

bool ResultSet::getBool(int index) noexcept(false) {
    bool value = false;
    if (DbError error = mImpl->getBooleanByIndex(index, value))
        error.raise();

    return value;
}

std::string_view ResultSet::getString(int index) noexcept(false) {
    std::string_view value;
    if (DbError error = mImpl->getStringByIndex(index, value))
        error.raise();

    return value;
}

Blob ResultSet::getBlob(int index) noexcept(false) {
    Blob value;
    if (DbError error = mImpl->getBlobByIndex(index, value))
        error.raise();

    return value;
}

double ResultSet::getDouble(std::string_view column) noexcept(false) {
    double value = 0.0;
    if (DbError error = mImpl->getDoubleByName(column, value))
        error.raise();

    return value;
}

int64 ResultSet::getInt(std::string_view column) noexcept(false) {
    int64 value = 0;
    if (DbError error = mImpl->getIntByName(column, value))
        error.raise();

    return value;
}

bool ResultSet::getBool(std::string_view column) noexcept(false) {
    bool value = false;
    if (DbError error = mImpl->getBooleanByName(column, value))
        error.raise();

    return value;
}

std::string_view ResultSet::getString(std::string_view column) noexcept(false) {
    std::string_view value;
    if (DbError error = mImpl->getStringByName(column, value))
        error.raise();

    return value;
}

Blob ResultSet::getBlob(std::string_view column) noexcept(false) {
    Blob value;
    if (DbError error = mImpl->getBlobByName(column, value))
        error.raise();

    return value;
}

///
/// named bindpoint
///

void BindPoint::toInt(int64 value) noexcept(false) {
    if (DbError error = mImpl->bindIntByName(mName, value))
        error.raise();
}

void BindPoint::toBool(bool value) noexcept(false) {
    if (DbError error = mImpl->bindBooleanByName(mName, value))
        error.raise();
}

void BindPoint::toString(std::string_view value) noexcept(false) {
    if (DbError error = mImpl->bindStringByName(mName, value))
        error.raise();
}

void BindPoint::toDouble(double value) noexcept(false) {
    if (DbError error = mImpl->bindDoubleByName(mName, value))
        error.raise();
}

void BindPoint::toBlob(Blob value) noexcept(false) {
    if (DbError error = mImpl->bindBlobByName(mName, value))
        error.raise();
}

void BindPoint::toNull() noexcept(false) {
    if (DbError error = mImpl->bindNullByName(mName))
        error.raise();
}

///
/// statement
///

BindPoint PreparedStatement::bind(std::string_view name) noexcept(false) {
    return BindPoint{mImpl.get(), name};
}

DbError PreparedStatement::close() noexcept {
    return mImpl->close();
}

DbError PreparedStatement::reset() noexcept {
    return mImpl->reset();
}

std::expected<ResultSet, DbError> PreparedStatement::select() noexcept {
    if (DbError error = mImpl->select())
        return std::unexpected(error);

    return ResultSet{mImpl};
}

std::expected<ResultSet, DbError> PreparedStatement::update() noexcept {
    if (DbError error = mImpl->update(mConnection->autoCommit()))
        return std::unexpected(error);

    return ResultSet{mImpl};
}

///
/// connection
///

bool Connection::tableExists(std::string_view name) noexcept(false) {
    bool exists = false;
    if (DbError error = mImpl->tableExists(name, exists))
        error.raise();

    return exists;
}

std::expected<Version, DbError> Connection::dbVersion() const noexcept {
    Version version;
    if (DbError error = mImpl->dbVersion(version))
        return std::unexpected(error);

    return version;
}

std::expected<PreparedStatement, DbError> Connection::sqlPrepare(std::string_view sql, StatementType type) noexcept {
    detail::IStatement *statement = nullptr;
    if (DbError error = mImpl->prepare(sql, &statement))
        return std::unexpected(error);

    return PreparedStatement{statement, this, type};
}

std::expected<PreparedStatement, DbError> Connection::dqlPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eQuery);
}

std::expected<PreparedStatement, DbError> Connection::dmlPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eModify);
}

std::expected<PreparedStatement, DbError> Connection::ddlPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eDefine);
}

std::expected<PreparedStatement, DbError> Connection::dclPrepare(std::string_view sql) noexcept {
    return sqlPrepare(sql, StatementType::eControl);
}

std::expected<ResultSet, DbError> Connection::select(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(dqlPrepare(sql));

    return stmt.select();
}

std::expected<ResultSet, DbError> Connection::update(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(dmlPrepare(sql));

    return stmt.update();
}

void Connection::begin() noexcept(false) {
    if (DbError error = mImpl->begin())
        error.raise();
}

void Connection::commit() noexcept(false) {
    if (DbError error = mImpl->commit())
        error.raise();
}

void Connection::rollback() noexcept(false) {
    if (DbError error = mImpl->rollback())
        error.raise();
}

///
/// environment
///

bool Environment::isSupported(DbType type) noexcept {
    switch (type) {
#define DB_TYPE(id, str, enabled) case DbType::id: return enabled;
#include "orm/orm.inc"
    }
}

std::string_view db::toString(DbType type) noexcept {
    switch (type) {
#define DB_TYPE(id, str, enabled) case DbType::id: return str;
#include "orm/orm.inc"
    }
}

std::expected<Environment, DbError> Environment::create(DbType type) noexcept {
    detail::IEnvironment *env = nullptr;
    DbError error = [&] {
        switch (type) {
        case DbType::eSqlite3: return detail::sqlite(&env);

#if ORM_HAS_POSTGRES
        case DbType::ePostgreSQL: return detail::postgres(&env);
#endif

#if ORM_HAS_MYSQL
        case DbType::eMySQL: return detail::mysql(&env);
#endif

#if ORM_HAS_ORCL
        case DbType::eOracleDB: return detail::oracledb(&env);
#endif

#if ORM_HAS_MSSQL
        case DbType::eMSSQL: return detail::mssql(&env);
#endif

#if ORM_HAS_DB2
        case DbType::eDB2: return detail::db2(&env);
#endif

#if ORM_HAS_ODBC
        case DbType::eODBC: return detail::odbc(&env);
#endif

        default: return DbError::unsupported("environment");
        }
    }();

    if (error)
        return std::unexpected(error);

    return Environment{env};
}

std::expected<Connection, DbError> Environment::connect(const ConnectionConfig& config) noexcept {
    detail::IConnection *connection = nullptr;
    if (DbError error = mImpl->connect(config, &connection))
        return std::unexpected(error);

    return Connection{connection, config};
}
