#pragma once

#include <sqlite3.h>

#include "drivers/common.hpp"

namespace sm::db::sqlite {
    class SqliteEnvironment final : public detail::IEnvironment {
        DbError execute(sqlite3 *db, const std::string& sql) noexcept;
        DbError pragma(sqlite3 *db, std::string_view option, std::string_view value) noexcept;

        bool close() noexcept override;
        detail::IConnection *connect(const ConnectionConfig& config) noexcept(false) override;

    public:
        SqliteEnvironment(const EnvConfig& config) noexcept;
    };
}
