#pragma once

#include "core/throws.hpp"
#include "orm/core.hpp"

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
        };

        DbError(int code, int status, std::string message) noexcept;

        int code() const noexcept { return mCode; }
        int status() const noexcept { return mStatus; }
        std::string_view message() const noexcept { return mMessage; }
        const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }

        void throwIfFailed() const throws(DbException);

        operator bool() const noexcept {
            return mStatus != eOk;
        }

        bool isSuccess() const noexcept {
            return mStatus == eOk;
        }

        bool isDone() const noexcept {
            return mStatus == eDone || mStatus == eError;
        }

        static DbError ok() noexcept;
        static DbError todo() noexcept;
        static DbError error(int code, std::string message) noexcept;
        static DbError done(int code) noexcept;
        static DbError unsupported(std::string_view subject) noexcept;
        static DbError columnNotFound(std::string_view column) noexcept;
        static DbError bindNotFound(std::string_view bind) noexcept;
        static DbError connectionError(std::string_view message) noexcept;
        static DbError notReady(std::string message) noexcept;
    };

    template<typename T>
    class DbResult : public std::expected<T, DbError> {
    public:
        using Super = std::expected<T, DbError>;
        using Super::Super;

        DbResult(Super&& result) noexcept
            : Super(std::move(result))
        { }

        DbResult(const Super& result) noexcept
            : Super(result)
        { }

        auto throwIfFailed() && throws(DbException) {
            if (!this->has_value())
                this->error().throwIfFailed();

            return std::move(this->value());
        }

        auto throwIfFailed() const&& throws(DbException) {
            if (!this->has_value())
                this->error().raise();

            return std::move(this->value());
        }

        template<typename F>
        DbResult<std::invoke_result_t<F, T>> transform(F&& f) & {
            if (this->has_value())
                return std::expected(std::invoke(std::forward<F>(f), this->value()));

            return std::unexpected(std::move(this->error()));
        }

        template<typename F>
        DbResult<std::invoke_result_t<F, T>> transform(F&& f) const& {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), this->value());

            return std::unexpected(std::move(this->error()));
        }

        template<typename F>
        DbResult<std::invoke_result_t<F, T>> transform(F&& f) && {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), std::move(this->value()));

            return std::unexpected(std::move(this->error()));
        }

        template<typename F>
        DbResult<std::invoke_result_t<F, T>> transform(F&& f) const&& {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), std::move(this->value()));

            return std::unexpected(std::move(this->error()));
        }

        template<typename F>
        std::invoke_result_t<F, T> and_then(F&& f) & {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), this->value());

            return std::unexpected(this->error());
        }

        template<typename F>
        std::invoke_result_t<F, T> and_then(F&& f) const& {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), this->value());

            return std::unexpected(this->error());
        }

        template<typename F>
        std::invoke_result_t<F, T> and_then(F&& f) && {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), std::move(this->value()));

            return std::unexpected(std::move(this->error()));
        }

        template<typename F>
        std::invoke_result_t<F, T> and_then(F&& f) const&& {
            if (this->has_value())
                return std::invoke(std::forward<F>(f), std::move(this->value()));

            return std::unexpected(std::move(this->error()));
        }

        constexpr auto operator<=>(const DbResult&) const = default;
    };

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

        int code() const noexcept { return mError.code(); }
        std::string_view message() const noexcept { return mMessage; }
        const std::stacktrace& stacktrace() const noexcept { return mError.stacktrace(); }

        virtual const char *what() const noexcept { return mMessage.c_str(); }
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
    };
}
