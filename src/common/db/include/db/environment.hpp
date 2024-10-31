#pragma once

#include "db/connection.hpp"

namespace sm::db {
    class Environment {
        friend Connection;

        detail::EnvHandle mImpl;

        Environment(detail::IEnvironment *impl) noexcept
            : mImpl(impl)
        { }

        detail::IEnvironment *impl() noexcept { return mImpl.get(); }

    public:
        SM_MOVE(Environment, default);

        [[nodiscard]]
        static bool isSupported(DbType type) noexcept;

        static DbResult<Environment> tryCreate(DbType type, const EnvConfig& config = EnvConfig{}) noexcept;

        static Environment create(DbType type, const EnvConfig& config = EnvConfig{});

        DbResult<Connection> tryConnect(const ConnectionConfig& config) noexcept;
        Connection connect(const ConnectionConfig& config);
    };
}
