#pragma once

#include <pg_config.h>
#include <libpq-fe.h>
#include <catalog/pg_type_d.h>

#include "drivers/common.hpp"

namespace sm::db::postgres {
    using PgConnectionHandle = std::unique_ptr<PGconn, decltype(&PQfinish)>;

    class PgConnection final : public detail::IConnection {
        PgConnectionHandle mConnection;

        DbError execute(const char *sql) noexcept;

        detail::IStatement *prepare(std::string_view sql) noexcept(false) override;

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
