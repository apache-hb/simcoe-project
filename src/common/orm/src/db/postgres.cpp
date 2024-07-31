#include "stdafx.hpp"

#include "common.hpp"

#include <libpq-fe.h>

using namespace sm;
using namespace sm::db;

static DbError getConnectionError(const PGconn *conn) noexcept {
    char *err = PQerrorMessage(conn);
    if (err == nullptr) {
        return DbError::ok();
    }

    std::string result{err};
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    std::replace(result.begin(), result.end(), '\n', ' ');

    return DbError::error(1, result);
}

class PgStatement : public detail::IStatement {

};

class PgConnection final : public detail::IConnection {
    PGconn *mConnection = nullptr;

    DbError close() noexcept override {
        PQfinish(mConnection);
        return DbError::ok();
    }

    DbError prepare(std::string_view sql, sqlite3_stmt **stmt) noexcept {
        return DbError::todo();
    }

    DbError prepare(std::string_view sql, detail::IStatement **statement) noexcept override {
        return DbError::todo();
    }

    DbError begin() noexcept override {
        return DbError::todo();
    }

    DbError commit() noexcept override {
        return DbError::todo();
    }

    DbError rollback() noexcept override {
        return DbError::todo();
    }

    DbError tableExists(std::string_view table, bool& exists) noexcept override {
        return DbError::todo();
    }

public:
    PgConnection(PGconn *conn) noexcept
        : mConnection(conn)
    { }
};

class PgEnvironment final : public detail::IEnvironment {
    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept override {
        std::string conn = fmt::format("postgresql://{}:{}@{}:{}/{}", config.user, config.password, config.host, config.port, config.database);
        PGconn *pgconn = PQconnectdb(conn.c_str());
        if (PQstatus(pgconn) != CONNECTION_OK) {
            DbError err = getConnectionError(pgconn);
            PQfinish(pgconn);
            return err;
        }

        *connection = new PgConnection{pgconn};

        return DbError::ok();
    }

    bool close() noexcept override {
        return false;
    }

public:
    PgEnvironment() noexcept = default;
};

DbError detail::postgres(IEnvironment **env) noexcept {
    static PgEnvironment instance;
    *env = &instance;
    return DbError::ok();
}
