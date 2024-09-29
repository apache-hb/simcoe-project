#pragma once

#include "core/throws.hpp"
#include "db/core.hpp"

#include <fmtlib/format.h>

#include <stacktrace>
#include <string>
#include <string_view>

namespace sm::db {
    class [[nodiscard("Possibly ignoring error")]] DbError {
        int mCode = 0;
        int mStatus = eOk;
        std::string mMessage;
        std::stacktrace mStacktrace;

    public:
        enum : int {
            eOk = 0,
            eError = 1,
            eUnimplemented = 2,
            eDone = 3,
            eConnectionError = 4,
            eNoData = 5,
        };

        DbError(int code, int status, std::string message, bool enableStackTrace = true) noexcept;

        int code() const noexcept { return mCode; }
        int status() const noexcept { return mStatus; }
        std::string_view message() const noexcept { return mMessage; }
        const char *what() const noexcept { return mMessage.c_str(); }
        const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }

        [[noreturn]]
        void raise() const throws(DbException);
        void throwIfFailed() const throws(DbException);

        operator bool() const noexcept {
            return mStatus != eOk && mStatus != eDone;
        }

        bool isSuccess() const noexcept {
            return mStatus == eOk || mStatus == eDone;
        }

        bool isDone() const noexcept {
            return mStatus == eDone || mStatus == eError;
        }

        static DbError ok() noexcept;
        static DbError todo() noexcept;
        static DbError todo(std::string_view subject) noexcept;
        static DbError error(int code, std::string message) noexcept;
        static DbError outOfMemory() noexcept;
        static DbError noData() noexcept;
        static DbError done(int code) noexcept;
        static DbError unsupported(std::string_view subject) noexcept;
        static DbError columnNotFound(std::string_view column) noexcept;
        static DbError typeMismatch(std::string_view column, DataType actual, DataType expected) noexcept;
        static DbError bindNotFound(std::string_view bind) noexcept;
        static DbError bindNotFound(int bind) noexcept;
        static DbError connectionError(std::string_view message) noexcept;
        static DbError notReady(std::string message) noexcept;
    };

    template<typename T>
    using DbResult = std::expected<T, DbError>;

    template<typename T>
    auto throwIfFailed(DbResult<T>&& result) throws(DbException) {
        if (!result)
            result.error().raise();

        return std::move(result.value());
    }

    class DbException : public std::exception {
        DbError mError;
        std::string mMessage;

    public:
        DbException(const DbError& error) noexcept
            : std::exception()
            , mError(error)
            , mMessage(error.message())
        { }

        DbException(const DbError& error, std::string_view context) noexcept
            : std::exception()
            , mError(error)
            , mMessage(fmt::format("{}. {}", error.message(), context))
        { }

        DbError error() const noexcept { return mError; }
        int code() const noexcept { return mError.code(); }
        std::string_view message() const noexcept { return mMessage; }
        const std::stacktrace& stacktrace() const noexcept { return mError.stacktrace(); }

        virtual const char *what() const noexcept { return mMessage.c_str(); }
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
