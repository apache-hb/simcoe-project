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

static Version getServerVersion(const oracle::OraServer& server, oracle::OraError& error) {
    text buffer[512];
    ub4 release;
    if (sword result = OCIServerRelease2(server, error, buffer, sizeof(buffer), OCI_HTYPE_SERVER, &release, OCI_DEFAULT))
        oraGetError(error, result).throwIfFailed();

    int major = OCI_SERVER_RELEASE_REL(release);
    int minor = OCI_SERVER_RELEASE_REL_UPD(release);
    int revision = OCI_SERVER_RELEASE_REL_UPD_REV(release);
    int increment = OCI_SERVER_RELEASE_REL_UPD_INC(release);
    int ext = OCI_SERVER_RELEASE_EXT(release);

    return Version{(const char*)buffer, major, minor, revision, increment, ext};
}

static ub2 getBoolColumnType(const Version& server) noexcept {
    // Oracle 23 introduced columns with the sql standard boolean type
    if (server.major >= 23)
        return SQLT_BOL;

    return SQLT_CHR;
}

static bool hasBoolColumnType(const Version& server) noexcept {
    return getBoolColumnType(server) == SQLT_BOL;
}

static detail::ConnectionInfo buildConnectionInfo(oracle::OraError& error, oracle::OraServer& server, oracle::OraService& service, oracle::OraSession& session) {
    Version clientVersion = getClientVersion();
    Version serverVersion = getServerVersion(server, error);
    // if boolean types arent available, we use a string of length 1
    // where '0' is false, and everything else is true (but we prefer '1')
    DataType boolType = hasBoolColumnType(serverVersion) ? DataType::eBoolean : DataType::eChar;

    return detail::ConnectionInfo {
        .clientVersion = clientVersion,
        .serverVersion = serverVersion,
        .boolType = boolType,
        .hasCommentOn = true,
        .hasNamedParams = true,
        .hasUsers = true,
        .hasTableSpaces = true,
        .hasSchemas = false,
        .hasDistinctTextTypes = true,
    };
}

oracle::OraStatement OraConnection::newStatement(std::string_view sql) {
    OraEnv& env = mEnvironment.env();

    OraResource<OraError> error = oraNewResource<OraError>(env);
    OraResource<OraStmt> statement = oraNewResource<OraStmt>(env, *error);

    if (sword result = OCIStmtPrepare2(mService, statement.address(), *error, (text*)sql.data(), sql.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT))
        throw DbException{oraGetError(*error, result)};

    return OraStatement{mEnvironment, *this, statement.release(), error.release()};
}

detail::IStatement *OraConnection::prepare(std::string_view sql) noexcept(false) {
    return new OraStatement{newStatement(sql)};
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
    return fmt::format("TRUNCATE TABLE {}", table.name);
}

std::string OraConnection::setupSelect(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupSelect(table);
}

std::string OraConnection::setupSelectByPrimaryKey(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupSelectByPrimaryKey(table);
}

std::string OraConnection::setupUpdate(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupUpdate(table);
}

std::string OraConnection::setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) {
    return oracle::setupSingletonTrigger(table.name);
}

std::string OraConnection::setupTableExists() noexcept(false) {
    return "SELECT COUNT(*) FROM SYS.ALL_TABLES WHERE table_name = UPPER(:name)";
}

std::string OraConnection::setupUserExists() noexcept(false) {
    return "SELECT COUNT(*) from SYS.ALL_USERS WHERE username = UPPER(:name)";
}

std::string OraConnection::setupTableSpaceExists() noexcept(false) {
    return "SELECT COUNT(*) FROM SYS.DBA_DATA_FILES WHERE tablespace_name = UPPER(:name)";
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

OraConnection::~OraConnection() noexcept {
    if (sword result = OCISessionEnd(mService, mError, mSession, OCI_DEFAULT)) {
        LOG_WARN(DbLog, "OCISessionEnd {}", oraGetError(mError, result));
    }

    if (sword result = OCIServerDetach(mServer, mError, OCI_DEFAULT)) {
        LOG_WARN(DbLog, "OCIServerDetach {}", oraGetError(mError, result));
    }

    if (DbError result = mSession.close(mError)) {
        LOG_WARN(DbLog, "Failed to close session: {}", result);
    }

    if (DbError result = mService.close(mError)) {
        LOG_WARN(DbLog, "Failed to close service: {}", result);
    }

    if (DbError result = mServer.close(mError)) {
        LOG_WARN(DbLog, "Failed to close server: {}", result);
    }

    if (DbError result = mError.close(nullptr)) {
        LOG_WARN(DbLog, "Failed to close error: {}", result);
    }
}

OraConnection::OraConnection(OraEnvironment& env, OraError error, OraServer server, OraService service, OraSession session)
    : detail::IConnection(buildConnectionInfo(error, server, service, session))
    , mEnvironment(env)
    , mError(error)
    , mServer(server)
    , mService(service)
    , mSession(session)
{ }

ub2 OraConnection::getBoolType() const noexcept {
    if (boolEquivalentType() == DataType::eBoolean)
        return SQLT_BOL;

    return SQLT_CHR;
}
