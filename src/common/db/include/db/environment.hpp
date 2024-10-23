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

        static Environment create(DbType type, const EnvConfig& config = EnvConfig{}) {
            return throwIfFailed(tryCreate(type, config));
        }

        DbResult<Connection> tryConnect(const ConnectionConfig& config) noexcept;
        Connection connect(const ConnectionConfig& config) {
            auto result = tryConnect(config);
            if (!result.has_value())
                throw DbConnectionException{result.error(), config};

            return std::move(*result);
        }
    };
}
