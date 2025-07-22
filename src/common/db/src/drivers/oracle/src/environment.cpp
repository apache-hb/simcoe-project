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

detail::IConnection *OraEnvironment::connect(const ConnectionConfig& config) noexcept(false) {

    /** Create base connection objects */
    OraResource<OraError> error = oraNewResource<OraError>(mEnv, nullptr);
    OraResource<OraServer> server = oraNewResource<OraServer>(mEnv, *error);

    /** Attach to the server */
    chrono::seconds timeout = chrono::duration_cast<chrono::seconds>(config.timeout);
    std::string conn = fmt::format(kConnectionString, timeout.count(), config.host, config.port, config.database);
    if (sword result = OCIServerAttach(*server, *error, (text*)conn.data(), conn.size(), OCI_DEFAULT)) {
        throw DbConnectionException{oraGetError(*error, result), config};
    }

    /** Create the service handle and attach the server connection */

    OraResource<OraService> service = oraNewResource<OraService>(mEnv, *error);

    if (sword result = (*service).setAttribute(*error, OCI_ATTR_SERVER, *server)) {
        throw DbConnectionException{oraGetError(*error, result), config};
    }

    /** Create session and configure it with connection details */

    OraResource<OraSession> session = oraNewResource<OraSession>(mEnv, *error);

    if (sword result = (*session).setString(*error, OCI_ATTR_USERNAME, config.user)) {
        throw DbConnectionException{oraGetError(*error, result), config};
    }

    if (sword result = (*session).setString(*error, OCI_ATTR_PASSWORD, config.password)) {
        throw DbConnectionException{oraGetError(*error, result), config};
    }

    if (sword result = (*session).setString(*error, OCI_ATTR_MODULE, kModuleName)) {
        LOG_WARN(DbLog, "Failed to set module name: {}", oraGetError(*error, result));
    }

    ub4 mode = getConnectMode(config);

    if (sword result = OCISessionBegin(*service, *error, *session, OCI_CRED_RDBMS, mode))
        throw DbConnectionException{oraGetError(*error, result), config};

    if (sword result = (*service).setAttribute(*error, OCI_ATTR_SESSION, *session))
        throw DbConnectionException{oraGetError(*error, result), config};

    return new OraConnection{*this, error.release(), server.release(), service.release(), session.release()};
}

OraEnvironment::~OraEnvironment() noexcept {
    OCITerminate(OCI_DEFAULT);
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

detail::IEnvironment *detail::newOracleEnvironment(const EnvConfig& config) {
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

        throw DbException{error};
    }

    orcl->attach(oci);

    return orcl.release();
}
