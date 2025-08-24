#pragma once

#include <pg_config.h>
#include <libpq-fe.h>
#include <catalog/pg_type_d.h>

#include "drivers/common.hpp"

namespace sm::db::postgres {
    using PgConnectionHandle = std::unique_ptr<PGconn, decltype(&PQfinish)>;

    class PgStatement final : public detail::IStatement {
        DbError start(bool autoCommit, StatementType type) noexcept override;

        DbError execute() noexcept override;

        DbError next() noexcept override;

        std::string getSql() const override;

    public:
        PgStatement(PGconn *conn) noexcept(false);
        ~PgStatement() noexcept override;
    };

    class PgConnection final : public detail::IConnection {
        PgConnectionHandle mConnection;

        DbError execute(const char *sql) noexcept;

        detail::IStatement *prepare(std::string_view sql) noexcept(false) override;

        std::string setupTableExists() noexcept(false) override;
        std::string setupUserExists() noexcept(false) override;

        DbError begin() noexcept override;
        DbError commit() noexcept override;
        DbError rollback() noexcept override;

    public:
        PgConnection(PgConnectionHandle connection);
    };

    class PgEnvironment final : public detail::IEnvironment {
        detail::IConnection *connect(const ConnectionConfig& config) noexcept(false) override;

        bool close() noexcept override {
            return false;
        }

    public:
        PgEnvironment() = default;
    };
}
