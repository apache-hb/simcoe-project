#include "stdafx.hpp"

#include "oracle/connection.hpp"
#include "oracle/environment.hpp"

using namespace sm::db;

using namespace std::string_literals;

namespace oracle = sm::db::oracle;
namespace chrono = std::chrono;

using OraConnection = oracle::OraConnection;
using OraEnvironment = oracle::OraEnvironment;

static Version getClientVersion() noexcept {
    sword major;
    sword minor;
    sword revision;
    sword increment;
    sword ext;
    OCIClientVersion(&major, &minor, &revision, &increment, &ext);

    return Version{"Oracle Call Interface", major, minor, revision, increment, ext};
}

DbResult<oracle::OraStatement> OraConnection::newStatement(std::string_view sql) noexcept {
    OraEnv& env = mEnvironment.env();

    OraResource<OraError> error = TRY_RESULT(oraNewResource<OraError>(env));
    OraResource<OraStmt> statement = TRY_RESULT(oraNewResource<OraStmt>(env, *error));

    if (sword result = OCIStmtPrepare2(mService, statement.address(), *error, (text*)sql.data(), sql.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT))
        return std::unexpected(oraGetError(*error, result));

    return OraStatement{mEnvironment, *this, statement.release(), error.release()};
}

DbError OraConnection::close() noexcept {
    if (sword result = OCISessionEnd(mService, mError, mSession, OCI_DEFAULT))
        return oraGetError(mError, result);

    if (sword result = OCIServerDetach(mServer, mError, OCI_DEFAULT))
        return oraGetError(mError, result);

    if (DbError result = mSession.close(mError))
        return result;

    if (DbError result = mService.close(mError))
        return result;

    if (DbError result = mServer.close(mError))
        return result;

    if (DbError result = mError.close(nullptr))
        return result;

    return DbError::ok();
}

DbError OraConnection::prepare(std::string_view sql, detail::IStatement **stmt) noexcept {
    OraStatement statement = TRY_UNWRAP(newStatement(sql));

    *stmt = new OraStatement{statement};

    return DbError::ok();
}

DbError OraConnection::begin() noexcept {
    // not needed on Oracle
    return DbError::ok();
}

DbError OraConnection::commit() noexcept {
    sword result = OCITransCommit(mService, mError, OCI_DEFAULT);
    return oraGetError(mError, result);
}

DbError OraConnection::rollback() noexcept {
    sword result = OCITransRollback(mService, mError, OCI_DEFAULT);
    return oraGetError(mError, result);
}

std::string OraConnection::setupInsert(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupInsert(table);
}

std::string OraConnection::setupInsertOrUpdate(const dao::TableInfo& table) noexcept(false) {
    if (table.uniqueKeys.size() > 1) {
        throw DbException{DbError::todo("Multiple unique keys not currently supported")};
    }

    return oracle::setupInsertOrUpdate(table);
}

std::string OraConnection::setupInsertReturningPrimaryKey(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupInsertReturningPrimaryKey(table);
}

std::string OraConnection::setupTruncate(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupTruncate(table.name);
}

std::string OraConnection::setupSelect(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupSelect(table);
}

std::string OraConnection::setupUpdate(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupUpdate(table);
}

std::string OraConnection::setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupSingletonTrigger(table.name);
}

std::string OraConnection::setupTableExists() noexcept(false) {
    return oracle::setupTableExists();
}

std::string OraConnection::setupCreateTable(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupCreateTable(table, hasBoolType());
}

std::string OraConnection::setupCommentOnTable(std::string_view table, std::string_view comment) noexcept(false) {
    return oracle::setupCommentOnTable(table, comment);
}

std::string OraConnection::setupCommentOnColumn(std::string_view table, std::string_view column, std::string_view comment) noexcept(false) {
    return oracle::setupCommentOnColumn(table, column, comment);
}

Version OraConnection::clientVersion() const noexcept {
    return getClientVersion();
}

Version OraConnection::serverVersion() const noexcept {
    return mServerVersion;
}

ub2 OraConnection::getBoolType() const noexcept {
    // Oracle 23 introduced columns with the sql standard boolean type
    if (mServerVersion.major >= 23)
        return SQLT_BOL;

    return SQLT_CHR;
}
