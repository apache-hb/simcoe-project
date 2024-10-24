#include "stdafx.hpp"

#include "oracle/environment.hpp"
#include "oracle/connection.hpp"

using namespace sm::db;

namespace chrono = std::chrono;

using OraEnvironment = sm::db::oracle::OraEnvironment;

static constexpr std::string_view kModuleName = "Simcoe DB Connector";

static constexpr std::string_view kConnectionString = R"(
    (DESCRIPTION=
        (CONNECT_TIMEOUT={})
        (ADDRESS=(PROTOCOL=TCP)(HOST={})(PORT={}))
        (CONNECT_DATA=(SERVICE_NAME={}))
    )
)";

static DbError getServerVersion(const oracle::OraServer& server, oracle::OraError& error, Version& version) noexcept {
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

static constexpr ub4 kRoleMode[(int)AssumeRole::eCount] = {
    [(int)AssumeRole::eDefault] = OCI_DEFAULT,
    [(int)AssumeRole::eSYSDBA]  = OCI_SYSDBA,
    [(int)AssumeRole::eSYSOPER] = OCI_SYSOPER,
    [(int)AssumeRole::eSYSASM]  = OCI_SYSASM,
    [(int)AssumeRole::eSYSBKP]  = OCI_SYSBKP,
    [(int)AssumeRole::eSYSDGD]  = OCI_SYSDGD,
    [(int)AssumeRole::eSYSKMT]  = OCI_SYSKMT,
    [(int)AssumeRole::eSYSRAC]  = OCI_SYSRAC,
};

static ub4 getConnectMode(const ConnectionConfig& config) noexcept {
    return kRoleMode[(int)config.role] | OCI_STMT_CACHE;
}

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

    if (sword result = (*session).setString(*error, OCI_ATTR_USERNAME, config.user))
        return oraGetError(*error, result);

    if (sword result = (*session).setString(*error, OCI_ATTR_PASSWORD, config.password))
        return oraGetError(*error, result);

    if (sword result = (*session).setString(*error, OCI_ATTR_MODULE, kModuleName)) {
        LOG_WARN(DbLog, "Failed to set module name: {}", oraGetError(*error, result));
    }

    ub4 mode = getConnectMode(config);

    if (sword result = OCISessionBegin(*service, *error, *session, OCI_CRED_RDBMS, mode))
        return oraGetError(*error, result);

    if (sword result = (*service).setAttribute(*error, OCI_ATTR_SESSION, *session))
        return oraGetError(*error, result);

    /** Get version information about the server */

    *connection = new OraConnection{*this, error.release(), server.release(), service.release(), session.release()};

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
    oracle::OraEnv oci;
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
        if (DbError error2 = oci.close(nullptr))
            LOG_WARN(DbLog, "Failed to close environment: {}", error2.message());

        return error;
    }

    orcl->attach(oci);

    *env = orcl.release();

    return DbError::ok();
}
