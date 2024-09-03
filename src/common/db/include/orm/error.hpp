#pragma once

#include "core/throws.hpp"
#include "orm/core.hpp"

#include <fmtlib/format.h>

#include <stacktrace>
#include <string>
#include <string_view>

namespace sm::db {
    class DbException;

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
            eNoMoreData = 3,
            eConnectionError = 4,
        };

        DbError(int code, int status, std::string message) noexcept;

        int code() const noexcept { return mCode; }
        int status() const noexcept { return mStatus; }
        std::string_view message() const noexcept { return mMessage; }
        const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }

        [[noreturn]]
        void raise() const throws(DbException);

        operator bool() const noexcept {
            return mStatus != eOk;
        }

        bool isSuccess() const noexcept {
            return mStatus == eOk;
        }

        bool isDone() const noexcept {
            return mStatus == eNoMoreData;
        }

        static DbError ok() noexcept;
        static DbError todo() noexcept;
        static DbError error(int code, std::string message) noexcept;
        static DbError noMoreData(int code) noexcept;
        static DbError unsupported(std::string_view subject) noexcept;
        static DbError columnNotFound(std::string_view column) noexcept;
        static DbError bindNotFound(std::string_view bind) noexcept;
        static DbError connectionError(std::string_view message) noexcept;
    };

    class DbException {
        int mCode = 0;
        std::string mMessage;
        std::stacktrace mStacktrace;

    public:
        DbException(const DbError& error) noexcept
            : mCode(error.code())
            , mMessage(error.message())
            , mStacktrace(error.stacktrace())
        { }

        DbException(const DbError& error, std::string_view context) noexcept
            : mCode(error.code())
            , mMessage(fmt::format("{}. {}", error.message(), context))
            , mStacktrace(error.stacktrace())
        { }

        int code() const noexcept { return mCode; }
        std::string_view message() const noexcept { return mMessage; }
        const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }

        virtual std::string what() const noexcept { return mMessage; }
    };

    class DbConnectionException : public DbException {
        ConnectionConfig mConfig;

    public:
        DbConnectionException(const DbError& error, const ConnectionConfig& config)
            : DbException(error, fmt::format("{}/[password]@{}:{}/{}", config.user, config.host, config.port, config.database))
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

        virtual std::string what() const noexcept override { return mStatement; }
    };
}
