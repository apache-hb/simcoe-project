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

int ResultSet::columnCount() const noexcept {
    return mImpl->columnCount();
}

double ResultSet::getDouble(int index) noexcept {
    double value = 0.0;
    if (DbError error = mImpl->getDouble(index, value))
        CT_NEVER("Failed to get double value: %s", error.message().data());

    return value;
}

int64 ResultSet::getInt(int index) noexcept {
    int64 value = 0;
    if (DbError error = mImpl->getInt(index, value))
        CT_NEVER("Failed to get int64 value: %s", error.message().data());

    return value;
}

bool ResultSet::getBool(int index) noexcept {
    bool value = false;
    if (DbError error = mImpl->getBoolean(index, value))
        CT_NEVER("Failed to get boolean value: %s", error.message().data());

    return value;
}

std::string_view ResultSet::getString(int index) noexcept {
    std::string_view value;
    if (DbError error = mImpl->getString(index, value))
        CT_NEVER("Failed to get string value: %s", error.message().data());

    return value;
}

Blob ResultSet::getBlob(int index) noexcept {
    Blob value;
    if (DbError error = mImpl->getBlob(index, value))
        CT_NEVER("Failed to get blob value: %s", error.message().data());

    return value;
}

///
/// bindpoint
///

BindPoint PreparedStatement::bind(std::string_view name) noexcept {
    int index = -1;
    if (DbError error = mImpl->getBindIndex(name, index)) {
        CT_NEVER("Failed to get bind index for parameter %s: %s", name.data(), error.message().data());
    }

    return BindPoint{mImpl.get(), index};
}

void BindPoint::to(int64 value) noexcept {
    if (DbError error = mImpl->bindInt(mIndex, value)) {
        CT_NEVER("Failed to bind int64 value %lld: %s", value, error.message().data());
    }
}

void BindPoint::to(bool value) noexcept {
    if (DbError error = mImpl->bindBoolean(mIndex, value)) {
        CT_NEVER("Failed to bind boolean value %s: %s", (value ? "true" : "false"), error.message().data());
    }
}

void BindPoint::to(std::string_view value) noexcept {
    if (DbError error = mImpl->bindString(mIndex, value)) {
        CT_NEVER("Failed to bind string value %s: %s", value.data(), error.message().data());
    }
}

void BindPoint::to(double value) noexcept {
    if (DbError error = mImpl->bindDouble(mIndex, value)) {
        CT_NEVER("Failed to bind double value %f: %s", value, error.message().data());
    }
}

void BindPoint::to(Blob value) noexcept {
    if (DbError error = mImpl->bindBlob(mIndex, value)) {
        CT_NEVER("Failed to bind blob value (length %zu): %s", value.size_bytes(), error.message().data());
    }
}

void BindPoint::to(std::nullptr_t) noexcept {
    if (DbError error = mImpl->bindNull(mIndex)) {
        CT_NEVER("Failed to bind null value: %s", error.message().data());
    }
}

///
/// statement
///

DbError PreparedStatement::close() noexcept {
    return mImpl->close();
}

DbError PreparedStatement::reset() noexcept {
    return mImpl->reset();
}

std::expected<ResultSet, DbError> PreparedStatement::select() noexcept {
    if (DbError error = mImpl->select())
        return std::unexpected(error);

    return ResultSet{mImpl, mEnv};
}

std::expected<ResultSet, DbError> PreparedStatement::update(bool commit) noexcept {
    if (DbError error = mImpl->update(commit))
        return std::unexpected(error);

    return ResultSet{mImpl, mEnv};
}

///
/// connection
///

bool Connection::tableExists(std::string_view name) noexcept {
    bool exists = false;
    if (DbError err = mImpl->tableExists(name, exists))
        CT_NEVER("Failed to check if table %s exists: %s", name.data(), err.message().data());

    return exists;
}

std::expected<PreparedStatement, DbError> Connection::prepare(std::string_view sql) noexcept {
    detail::IStatement *statement = nullptr;
    if (DbError error = mImpl->prepare(sql, &statement))
        return std::unexpected(error);

    return PreparedStatement{statement, mEnv};
}

std::expected<ResultSet, DbError> Connection::select(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(prepare(sql));

    return stmt.select();
}

std::expected<ResultSet, DbError> Connection::update(std::string_view sql) noexcept {
    PreparedStatement stmt = TRY_RESULT(prepare(sql));

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

// NOLINTBEGIN
bool Environment::isSupported(DbType type) noexcept {
    switch (type) {
    case DbType::eSqlite3: return true;
    case DbType::ePostgreSQL: return HAS_POSTGRES;
    case DbType::eMySQL: return HAS_MYSQL;
    case DbType::eOracleDB: return HAS_ORACLEDB;
    case DbType::eMSSQL: return HAS_MSSQL;
    case DbType::eDB2: return HAS_DB2;
    }
}
// NOLINTEND

std::expected<Environment, DbError> Environment::create(DbType type) noexcept {
    detail::IEnvironment *env = nullptr;
    DbError error = [&] {
        switch (type) {
        case DbType::eSqlite3: return detail::sqlite(&env);
#if HAS_POSTGRES
        case DbType::ePostgreSQL: return detail::postgres(&env);
#endif

#if HAS_MYSQL
        case DbType::eMySQL: return detail::mysql(&env);
#endif

#if HAS_ORACLEDB
        case DbType::eOracleDB: return detail::oracledb(&env);
#endif

#if HAS_MSSQL
        case DbType::eMSSQL: return detail::mssql(&env);
#endif

#if HAS_DB2
        case DbType::eDB2: return detail::db2(&env);
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

    return Connection{connection, mImpl.get()};
}
