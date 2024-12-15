#include "stdafx.hpp"

#include "common.hpp"

#include <array>

#include <libpq-fe.h>
#include <catalog/pg_type_d.h>

using namespace sm;
using namespace sm::db;

namespace chrono = std::chrono;

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

class PgResult {
    PgResultHandle mHandle;

public:
    SM_NOCOPY(PgResult);

    PgResult(PGresult *result) noexcept
        : mHandle(result, &PQclear)
    { }

    operator PGresult*() const noexcept {
        return mHandle.get();
    }
};

class PgStatement final : public detail::IStatement {
    PGconn *mConnection = nullptr;
    std::string mStmtName;

    DbError select() noexcept {
        PgResult result = PQexecPrepared(mConnection, mStmtName.c_str(), 0, nullptr, nullptr, nullptr, 0);
        if (DbError error = getResultError(result))
            return error;

        return DbError::todo();
    }

public:
    PgStatement(PGconn *conn, std::string name) noexcept
        : mConnection(conn)
        , mStmtName(std::move(name))
    { }
};

class PgConnection final : public detail::IConnection {
    PGconn *mConnection = nullptr;

    int mCounter = 0;

    DbError execute(std::string_view sql) noexcept {
        PgResult result = PQexec(mConnection, sql.data());

        if (DbError error = getResultError(result))
            return error;

        return DbError::ok();
    }

    DbError executeWithBindings(std::string_view sql, std::span<const Oid> types, std::span<const void*> params, std::span<const int> lengths, std::span<const int> formats) noexcept {
        PgResult result = PQexecParams(
            mConnection, sql.data(), /* connection + query */
            types.size(), types.data(), /* parameter types */
            (const char**)params.data(), lengths.data(), /* parameter data */
            formats.data(), /* parameter formats */
            1 /* binary results */
        );

        if (DbError error = getResultError(result))
            return error;

        return DbError::ok();
    }

    DbError close() noexcept override {
        PQfinish(mConnection);
        return DbError::ok();
    }

    DbError prepare(std::string_view sql, detail::IStatement **statement) noexcept override {
        std::string name = fmt::format("stmt{}", mCounter++);
        PgResult result = PQprepare(mConnection, name.c_str(), sql.data(), 0, nullptr);
        if (DbError error = getResultError(result))
            return error;

        *statement = new PgStatement{mConnection, name};

        return DbError::todo();
    }

    DbError begin() noexcept override {
        return execute("BEGIN TRANSACTION");
    }

    DbError commit() noexcept override {
        return execute("COMMIT TRANSACTION");
    }

    DbError rollback() noexcept override {
        return execute("ROLLBACK TRANSACTION");
    }

    DbError tableExists(std::string_view name, bool& exists) noexcept override {
        std::string_view sql = "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = :name)";
        std::array types = std::to_array<Oid>({ TEXTOID });
        std::array data = std::to_array<const void*>({ name.data() });
        std::array lengths = std::to_array<int>({ static_cast<int>(name.size()) });
        std::array formats = std::to_array<int>({ 1 });

        if (DbError error = executeWithBindings(sql, types, data, lengths, formats))
            return error;

        return DbError::todo();
    }

public:
    PgConnection(PGconn *conn) noexcept
        : mConnection(conn)
    { }
};

class PgEnvironment final : public detail::IEnvironment {
    DbError connect(const ConnectionConfig& config, detail::IConnection **connection) noexcept(false) override {
        auto seconds = chrono::duration_cast<chrono::seconds>(config.timeout).count();
        std::string conn = fmt::format("postgresql://{}:{}@{}:{}/{}?connect_timeout={}", config.user, config.password, config.host, config.port, config.database, seconds);
        PGconn *pgconn = PQconnectdb(conn.c_str());
        if (DbError err = getConnectionError(pgconn)) {
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

DbError detail::getPostgresEnv(IEnvironment **env) noexcept {
    static PgEnvironment instance;
    *env = &instance;
    return DbError::ok();
}
