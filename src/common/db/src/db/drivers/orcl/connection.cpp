#include "stdafx.hpp"

#include "orcl.hpp"

#include "core/defer.hpp"

using namespace sm::db;
using namespace sm::db::detail::orcl;

namespace chrono = std::chrono;

static DbError getServerVersion(const OraServer& server, OraError& error, Version& version) noexcept {
    text buffer[512];
    ub4 release;
    if (sword result = OCIServerRelease2(server, error, buffer, sizeof(buffer), OCI_HTYPE_SERVER, &release, OCI_DEFAULT))
        return oraGetError(error, result);

    int major = OCI_SERVER_RELEASE_REL(release);
    int minor = OCI_SERVER_RELEASE_REL_UPD(release);
    int revision = OCI_SERVER_RELEASE_REL_UPD_REV(release);
    int increment = OCI_SERVER_RELEASE_REL_UPD_INC(release);
    int ext = OCI_SERVER_RELEASE_EXT(release);

    version = Version{(const char*)buffer, major, minor, revision, increment, ext};

    return DbError::ok();
}

static Version getClientVersion() noexcept {
    sword major;
    sword minor;
    sword revision;
    sword increment;
    sword ext;
    OCIClientVersion(&major, &minor, &revision, &increment, &ext);

    return Version{"Oracle Call Interface", major, minor, revision, increment, ext};
}

DbResult<OraStatement> OraConnection::newStatement(std::string_view sql) noexcept {
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
    return orcl::setupInsert(table);
}

std::string OraConnection::setupInsertOrUpdate(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupInsertOrUpdate(table);
}

std::string OraConnection::setupInsertReturningPrimaryKey(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupInsertReturningPrimaryKey(table);
}

std::string OraConnection::setupTruncate(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupTruncate(table.name);
}

std::string OraConnection::setupSelect(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupSelect(table);
}

std::string OraConnection::setupUpdate(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupUpdate(table);
}

std::string OraConnection::setupSingletonTrigger(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupSingletonTrigger(table.name);
}

std::string OraConnection::setupTableExists() noexcept(false) {
    return orcl::setupTableExists();
}

std::string OraConnection::setupCreateTable(const dao::TableInfo& table) noexcept(false) {
    return orcl::setupCreateTable(table, hasBoolType());
}

DbError OraConnection::clientVersion(Version& version) const noexcept {
    version = getClientVersion();
    return DbError::ok();
}

DbError OraConnection::serverVersion(Version& version) const noexcept {
    version = mServerVersion;
    return DbError::ok();
}

ub2 OraConnection::getBoolType() const noexcept {
    // Oracle 23ai introduced columns with the sql standard boolean type
    if (mServerVersion.major >= 23)
        return SQLT_BOL;

    return SQLT_CHR;
}

static constexpr std::string_view kConnectionString = R"(
    (DESCRIPTION=
        (CONNECT_TIMEOUT={})
        (ADDRESS=(PROTOCOL=TCP)(HOST={})(PORT={}))
        (CONNECT_DATA=(SERVICE_NAME={}))
    )
)";

DbError OraEnvironment::connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept {

    /** Create base connection objects */
    OraResource<OraError> error = TRY_UNWRAP(oraNewResource<OraError>(mEnv, nullptr));
    OraResource<OraServer> server = TRY_UNWRAP(oraNewResource<OraServer>(mEnv, *error));

    /** Attach to the server */
    chrono::seconds timeout = chrono::duration_cast<chrono::seconds>(config.timeout);
    std::string conn = fmt::format(kConnectionString, timeout.count(), config.host, config.port, config.database);
    if (sword result = OCIServerAttach(*server, *error, (text*)conn.data(), conn.size(), OCI_DEFAULT))
        return oraGetError(*error, result);

    /** Create the service handle and attach the server connection */

    OraResource<OraService> service = TRY_UNWRAP(oraNewResource<OraService>(mEnv, *error));

    if (sword result = (*service).setAttribute(*error, OCI_ATTR_SERVER, *server))
        return oraGetError(*error, result);

    /** Create session and configure it with connection details */

    OraResource<OraSession> session = TRY_UNWRAP(oraNewResource<OraSession>(mEnv, *error));

    if (sword result = (*session).setAttribute(*error, OCI_ATTR_USERNAME, config.user))
        return oraGetError(*error, result);

    if (sword result = (*session).setAttribute(*error, OCI_ATTR_PASSWORD, config.password))
        return oraGetError(*error, result);

    if (sword result = OCISessionBegin(*service, *error, *session, OCI_CRED_RDBMS, OCI_STMT_CACHE))
        return oraGetError(*error, result);

    if (sword result = (*service).setAttribute(*error, OCI_ATTR_SESSION, *session))
        return oraGetError(*error, result);

    /** Get version information about the server */

    Version version;
    if (DbError result = getServerVersion(*server, *error, version))
        return result;

    *connection = new OraConnection{*this, error.release(), server.release(), service.release(), session.release(), version};

    return DbError::ok();
}

void *OraEnvironment::malloc(size_t size) noexcept {
    void *ptr = std::malloc(size);
    return ptr;
}

void *OraEnvironment::realloc(void *ptr, size_t size) noexcept {
    void *result = std::realloc(ptr, size);
    return result;
}

void OraEnvironment::free(void *ptr) noexcept {
    std::free(ptr);
}

void *OraEnvironment::wrapMalloc(void *ctx, size_t size) {
    auto env = static_cast<OraEnvironment*>(ctx);
    return env->malloc(size);
}

void *OraEnvironment::wrapRealloc(void *ctx, void *ptr, size_t size) {
    auto env = static_cast<OraEnvironment*>(ctx);
    return env->realloc(ptr, size);
}

void OraEnvironment::wrapFree(void *ctx, void *ptr) {
    auto env = static_cast<OraEnvironment*>(ctx);
    env->free(ptr);
}

DbError detail::getOracleEnv(detail::IEnvironment **env, const EnvConfig& config) noexcept {
    UniquePtr orcl = makeUnique<OraEnvironment>();
    OraEnv oci;
    sword result = OCIEnvNlsCreate(
        &oci, OCI_THREADED, orcl.get(),
        &OraEnvironment::wrapMalloc,
        &OraEnvironment::wrapRealloc,
        &OraEnvironment::wrapFree,
        0, nullptr,
        0, 0
    );

    if (result != OCI_SUCCESS) {
        DbError error = oraGetHandleError(oci, result, OCI_HTYPE_ENV);
        (void)oci.close(nullptr); // TODO: log if this errors
        return error;
    }

    orcl->attach(oci);

    *env = orcl.release();

    return DbError::ok();
}
