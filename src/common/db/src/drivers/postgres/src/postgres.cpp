#include "stdafx.hpp"

#include "postgres/postgres.hpp"

using namespace sm;
using namespace sm::db;

namespace chrono = std::chrono;

[[maybe_unused]]
static DbError getResultError(const PGresult *result) noexcept {
    if (result == nullptr)
        return DbError::outOfMemory();

    int status = PQresultStatus(result);
    if (status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK)
        return DbError::ok();

    char *err = PQresultErrorMessage(result);
    if (err == nullptr)
        return DbError::ok();

    std::string text{err};
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    std::replace(text.begin(), text.end(), '\n', ' ');

    return DbError::error(status, text);
}

static DbError getConnectionError(const PGconn *conn) noexcept {
    int status = PQstatus(conn);
    if (status == CONNECTION_OK)
        return DbError::ok();

    char *err = PQerrorMessage(conn);
    if (err == nullptr)
        return DbError::ok();

    std::string text{err};
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    std::replace(text.begin(), text.end(), '\n', ' ');

    return DbError::error(status, text);
}

using PgResultHandle = std::unique_ptr<PGresult, decltype(&PQclear)>;
using PgConnectionHandle = std::unique_ptr<PGconn, decltype(&PQfinish)>;

class PgResult {
    PgResultHandle mHandle;

public:
    PgResult(PGresult *result) noexcept
        : mHandle(result, &PQclear)
    { }

    operator PGresult*() const noexcept {
        return mHandle.get();
    }
};

static Version parseVersion(const char *name, int version) {
    int major = (version / 10000) % 100;
    int minor = (version / 100) % 100;

    return Version{name, major, minor, 0};
}

static Version getLibpqVersion() {
    return parseVersion(PG_VERSION_STR, PQlibVersion());
}

static Version getDbVersion(const PGconn *conn) {
    int version = PQserverVersion(conn);
    const char *name = PQparameterStatus(conn, "server_version");
    if (name == nullptr) {
        LOG_WARN(DbLog, "Failed to find parameter 'server_version'");
        name = PACKAGE_NAME " (unknown version)";
    }

    return parseVersion(name, version);
}

static detail::ConnectionInfo buildConnectionInfo(const PGconn *conn) {
    return detail::ConnectionInfo {
        .clientVersion = getLibpqVersion(),
        .serverVersion = getDbVersion(conn),
        .boolType = DataType::eInteger,
        .dateTimeType = DataType::eInteger,
        .hasCommentOn = false,
        .hasNamedParams = false,
        .hasUsers = false,
        .permissions = Permission::eAll,
    };
}

DbError postgres::PgConnection::execute(const char *sql) noexcept {
    PgResult result = PQexec(mConnection.get(), sql);
    if (DbError error = getResultError(result))
        return error;

    return DbError::ok();
}

detail::IStatement *postgres::PgConnection::prepare(std::string_view sql) noexcept(false) {
    return nullptr;
}

DbError postgres::PgConnection::begin() noexcept {
    return execute("BEGIN TRANSACTION");
}

DbError postgres::PgConnection::commit() noexcept {
    return execute("COMMIT TRANSACTION");
}

DbError postgres::PgConnection::rollback() noexcept {
    return execute("ROLLBACK TRANSACTION");
}

postgres::PgConnection::PgConnection(PgConnectionHandle connection)
    : IConnection(buildConnectionInfo(connection.get()))
    , mConnection(std::move(connection))
{ }

static std::string buildConnectionString(const ConnectionConfig& config) {
    if (!config.connection.empty()) {
        return config.connection;
    }

    std::stringstream ss;
    ss << "postgresql://";
    bool userspec = !config.user.empty();
    bool hostspec = !config.host.empty();
    bool dbname = !config.database.empty();
    if (userspec) {
        ss << config.user;

        if (!config.password.empty()) {
            ss << ":" << config.password;
        }
    }

    if (hostspec) {
        if (userspec)
            ss << "@";

        ss << config.host << ":" << config.port;
    }

    if (dbname) {
        if (userspec || hostspec) {
            ss << "/";
        }

        ss << config.database;
    }

    long long seconds = chrono::duration_cast<chrono::seconds>(config.timeout).count();
    ss << "?connect_timeout=" << seconds;

    return ss.str();
}

detail::IConnection *postgres::PgEnvironment::connect(const ConnectionConfig& config) noexcept(false) {
    std::string conn = buildConnectionString(config);
    PgConnectionHandle pgconn = PgConnectionHandle(PQconnectdb(conn.c_str()), &PQfinish);
    if (DbError err = getConnectionError(pgconn.get())) {
        throw DbConnectionException{err, config};
    }

    return new PgConnection{std::move(pgconn)};
}

detail::IEnvironment *detail::newPostgresEnvironment(const EnvConfig& config) {
    static postgres::PgEnvironment sEnvironment;
    return &sEnvironment;
}
