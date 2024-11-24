#pragma once

#include "core/throws.hpp"

#include <expected>
#include <stacktrace>
#include <string>

#include <fmtlib/format.h>

namespace sm::errors {
    class [[nodiscard("Ignoring error value")]] AnyError {
        using Self = AnyError;

        std::string mMessage;
        std::stacktrace mStacktrace;

    protected:
        AnyError(std::string message, bool trace = true);

    public:
        AnyError();

        std::string_view message() const noexcept { return mMessage; }
        const char *what() const noexcept { return mMessage.c_str(); }
        const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }
    };

    template<typename T>
    class Error : public AnyError {
        friend T;

        using AnyError::AnyError;

        constexpr Error() = default;

        constexpr T& self() noexcept { return static_cast<T&>(*this); }
        constexpr const T& self() const noexcept { return static_cast<const T&>(*this); }

        constexpr bool isSuccessImpl() const noexcept {
            return self().isSuccess();
        }

    public:
        [[noreturn]]
        void raise() const throws(typename T::Exception) {
            throw (typename T::Exception){ self() };
        }

        void throwIfFailed() const throws(typename T::Exception) {
            if (!isSuccessImpl())
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

        /// evaluates to true if an error is present
        /// decided to do this to get the short error handling syntax
        /// @code{.cpp}
        /// if (Error err = maybe()) {
        ///     // handle error
        /// }
        /// @endcode
        constexpr operator bool() const noexcept {
            return !isSuccessImpl();
        }
    };

    template<typename T>
    concept IsError = std::is_base_of_v<AnyError, T>;

    class AnyException : public std::runtime_error {
        using Super = std::runtime_error;
    public:
        using Super::Super;

        virtual const std::stacktrace& stacktrace() const noexcept = 0;
    };

    template<IsError T>
    class Exception : public AnyException {
        T mError;

    public:
        Exception(T error, const std::string& message)
            : AnyException(message)
            , mError(std::move(error))
        { }

        Exception(T error)
            : AnyException(error.what()) // TODO: copies
            , mError(std::move(error))
        { }

        const char *what() const noexcept override { return mError.what(); }

        const T& error() const noexcept { return mError; }
        const std::stacktrace& stacktrace() const noexcept override final { return mError.stacktrace(); }
    };

    template<typename T>
    concept IsException = std::is_base_of_v<AnyException, T>;
}

template<sm::errors::IsError T>
struct fmt::formatter<T> {
    constexpr auto parse(fmt::format_parse_context& ctx) const noexcept {
        return ctx.begin();
    }

    constexpr auto format(const T& error, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}", error.message());
    }
};
