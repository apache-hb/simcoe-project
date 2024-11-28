#pragma once

#include "core/throws.hpp"
#include "core/error/error.hpp"

#include "db/db.hpp"

#include <fmtlib/format.h>

#include <string>
#include <string_view>

namespace sm::db {
    class DbError : public errors::Error<DbError> {
        using Super = errors::Error<DbError>;

        int mCode = 0;
        int mStatus = eOk;

    public:
        using Exception = class DbException;

        enum : int {
            eOk = 0,
            eError = 1,
            eUnimplemented = 2,
            eDone = 3,
            eConnectionError = 4,
            eNoData = 5,
        };

        DbError(int code, int status, std::string message, bool trace = true);

        int code() const noexcept { return mCode; }
        int status() const noexcept { return mStatus; }

        bool isSuccess() const noexcept {
            return mStatus == eOk || mStatus == eDone;
        }

        bool isDone() const noexcept {
            return mStatus == eDone || mStatus == eError;
        }

        static DbError ok();
        static DbError todo();
        static DbError todo(std::string_view subject);
        static DbError todoFn(std::source_location location = std::source_location::current());
        static DbError error(int code, std::string message);
        static DbError outOfMemory();
        static DbError noData();
        static DbError columnIsNull(std::string_view name);
        static DbError done(int code);
        static DbError unsupported(std::string_view subject);
        static DbError unsupported(DbType type);
        static DbError columnNotFound(std::string_view column);
        static DbError typeMismatch(std::string_view column, DataType actual, DataType expected);
        static DbError bindNotFound(std::string_view bind);
        static DbError bindNotFound(int bind);
        static DbError connectionError(std::string_view message);
        static DbError notReady(std::string message);
        static DbError invalidHost(std::string_view reason);
    };

    template<typename T>
    using DbResult = DbError::Result<T>;

    template<typename T>
    auto throwIfFailed(DbResult<T>&& result) throws(DbException) {
        if (!result)
            result.error().raise();

        return std::move(result.value());
    }

    class DbException : public errors::Exception<DbError> {
        using Super = errors::Exception<DbError>;
    public:
        using Super::Super;

        int code() const noexcept { return error().code(); }
    };

    class DbConnectionException : public DbException {
        ConnectionConfig mConfig;

    public:
        DbConnectionException(const DbError& error, const ConnectionConfig& config)
            : DbException(error, fmt::format("{}/pwd@{}:{}/{}", config.user, config.host, config.port, config.database))
            , mConfig(config)
        { }

        const ConnectionConfig& config() const noexcept { return mConfig; }
    };

    class DbExecuteException : public DbException {
        std::string mStatement;

    public:
        DbExecuteException(const DbError& error, std::string stmt)
            : DbException(error, stmt)
            , mStatement(std::move(stmt))
        { }

        std::string_view statement() const noexcept { return mStatement; }
    };
}

template<>
struct fmt::formatter<sm::db::DbError> {
    constexpr auto parse(fmt::format_parse_context& ctx) const {
        return ctx.begin();
    }

    constexpr auto format(const sm::db::DbError& error, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "DbError({}: {})", error.code(), error.message());
    }
};
