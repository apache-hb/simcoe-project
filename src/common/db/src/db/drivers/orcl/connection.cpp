#include "stdafx.hpp"

#include "orcl.hpp"

#include "core/defer.hpp"

using namespace sm::db;
using namespace sm::db::detail::orcl;

static DbError getServerVersion(OraServer& server, OraError& error, Version& version) noexcept {
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
    OraStmt statement;
    OraError error;
    OraEnv& env = mEnvironment.env();
    if (DbError result = oraNewHandle(env, error))
        return std::unexpected(result);

    if (DbError result = oraNewHandle(env, statement))
        return std::unexpected(result);

    if (sword result = OCIStmtPrepare2(mService, &statement, error, (text*)sql.data(), sql.size(), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT))
        return std::unexpected(oraGetError(error, result));

    return OraStatement{mEnvironment, *this, statement, error};
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

DbError OraConnection::setupInsert(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = orcl::setupInsert(table);
    return DbError::ok();
}

DbError OraConnection::setupInsertOrUpdate(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = orcl::setupInsertOrUpdate(table);
    return DbError::ok();
}

DbError OraConnection::setupInsertReturningPrimaryKey(const dao::TableInfo& table, std::string& sql) noexcept {
    sql = orcl::setupInsertReturningPrimaryKey(table);
    return DbError::ok();
}

DbError OraConnection::createTable(const dao::TableInfo& table) noexcept {
    auto sql = orcl::setupCreateTable(table);
    fmt::println(stderr, "Creating table: {}", sql);
    OraStatement stmt = TRY_UNWRAP(newStatement(sql));
    defer { (void)stmt.finalize(); };

    return stmt.update(true);
}

DbError OraConnection::tableExists(std::string_view name, bool& exists) noexcept {
    OraStatement stmt = TRY_UNWRAP(newStatement("SELECT COUNT(*) FROM user_tables WHERE table_name = UPPER(:1)"));
    defer { (void)stmt.finalize(); };

    if (DbError error = stmt.bindStringByIndex(0, name))
        return error;

    if (DbError result = stmt.select())
        return result;

    if (DbError result = stmt.next())
        return result;

    int64 count;
    if (DbError result = stmt.getIntByIndex(0, count))
        return result;

    exists = count > 0;

    return stmt.finalize();
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
    if constexpr (!kHasBoolType)
        return SQLT_CHR;

    // Oracle 23ai introduced the sql standard boolean type
    if (mServerVersion.major >= 23)
        return kBoolType;

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
    OraServer server;
    OraError error;
    OraService service;
    OraSession session;
    Version version;

    if (DbError result = oraNewHandle(mEnv, error))
        return result;

    if (DbError result = oraNewHandle(mEnv, server))
        return result;

    /** Attach to the server */
    std::string conn = fmt::format(kConnectionString, config.timeout.count(), config.host, config.port, config.database);
    if (sword result = OCIServerAttach(server, error, (text*)conn.data(), conn.size(), OCI_DEFAULT))
        return oraGetError(error, result);

    if (DbError result = oraNewHandle(mEnv, service))
        return result;

    if (sword result = service.setAttribute(error, OCI_ATTR_SERVER, server))
        return oraGetError(error, result);

    if (DbError result = oraNewHandle(mEnv, session))
        return result;

    if (sword result = session.setAttribute(error, OCI_ATTR_USERNAME, config.user))
        return oraGetError(error, result);

    if (sword result = session.setAttribute(error, OCI_ATTR_PASSWORD, config.password))
        return oraGetError(error, result);

    if (sword result = OCISessionBegin(service, error, session, OCI_CRED_RDBMS, OCI_STMT_CACHE))
        return oraGetError(error, result);

    if (sword result = service.setAttribute(error, OCI_ATTR_SESSION, session))
        return oraGetError(error, result);

    if (DbError result = getServerVersion(server, error, version))
        return result;

    *connection = new OraConnection{*this, error, server, service, session, version};

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

DbError detail::getOracleEnv(detail::IEnvironment **env) noexcept {
    UniquePtr orcl = makeUnique<OraEnvironment>();
    OraEnv oci;
    sword result = OCIEnvCreate(
        &oci, OCI_THREADED, orcl.get(),
        &OraEnvironment::wrapMalloc,
        &OraEnvironment::wrapRealloc,
        &OraEnvironment::wrapFree,
        0, nullptr
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
