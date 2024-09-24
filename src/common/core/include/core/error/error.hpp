#pragma once

#include "core/adt/zstring_view.hpp"
#include "core/throws.hpp"

#include <exception>
#include <expected>
#include <stacktrace>
#include <string>

namespace sm::errors {
    class AnyError;

    class AnyException;

    template<typename T>
    concept IsError = std::is_base_of_v<AnyError, T>;

    template<typename T>
    concept IsException = std::is_base_of_v<AnyException, T>;

    class [[nodiscard("Ignoring error value")]] AnyError {
        using Self = AnyError;

        std::string mMessage;
        std::stacktrace mStacktrace;

    protected:
        AnyError(std::string message, bool trace = true) noexcept;

    public:
        static constexpr int kSuccess = 0;

        [[nodiscard]] sm::ZStringView message() const noexcept { return mMessage; }
        [[nodiscard]] const char *what() const noexcept { return mMessage.c_str(); }
        [[nodiscard]] const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }
    };

    template<typename T>
    class [[nodiscard("Ignoring error value")]] Error : public AnyError {
        friend T;

        using AnyError::AnyError;
        constexpr Error() = default;

        constexpr T& self() noexcept { return static_cast<T&>(*this); }
        constexpr const T& self() const noexcept { return static_cast<const T&>(*this); }
    public:

        bool isSuccess() const noexcept {
            return self().isSuccessImpl();
        }

        CT_NORETURN raise() const throws(typename T::Exception) {
            throw (typename T::Exception){ *this };
        }

        void throwIfFailed() const throws(typename T::Exception) {
            if (!isSuccess())
                raise();
        }

        template<typename V>
        using Result = std::expected<V, T>;

        template<typename V>
        static V orThrow(Result<V>&& result) throws(Exception) {
            if (!result.has_value())
                result.error().raise();

            return std::move(result.value());
        }
    };

    class AnyException : public std::exception {
        std::string mMessage;
        std::stacktrace mStacktrace;

    protected:
        AnyException(std::string message, std::stacktrace stacktrace) noexcept
            : mMessage(std::move(message))
            , mStacktrace(std::move(stacktrace))
        { }

    public:
        [[nodiscard]] const char *what() const noexcept override { return mMessage.c_str(); }
        [[nodiscard]] const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }
    };

    template<typename T>
    class Exception : public AnyException {
        T mError;

    public:
        Exception(T error)
            : AnyException(error.message(), error.stacktrace())
            , mError(std::move(error))
        { }

        [[nodiscard]] const T& error() const noexcept { return mError; }
    };

    class DbError : public Error<DbError> {
    public:
    };

    class DbException : public Exception<DbError> {

    };

    class NetError : public Error<NetError> {
    public:
    };

    class NetException : public Exception<NetError> {

    };
}
