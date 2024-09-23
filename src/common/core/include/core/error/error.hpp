#pragma once

#include "core/adt/zstring_view.hpp"
#include "core/throws.hpp"

#include <exception>
#include <expected>
#include <stacktrace>
#include <string>

namespace sm {
    class AnyError;

    template<typename T>
    concept IsError = std::is_base_of_v<AnyError, T>;

    template<IsError T>
    class AnyException;

    template<typename T>
    concept IsException = std::is_base_of_v<AnyException<T>, T>;

    class [[nodiscard("Ignoring error value")]] AnyError {
        using Self = AnyError;

        int mCode;
        std::string mMessage;
        std::stacktrace mStacktrace;

    protected:
        AnyError(int code, std::string message, bool trace = true) noexcept;
        AnyError(int code, std::string_view message, bool trace = true);

    public:
        enum : int { eOk = 0 };

        [[nodiscard]] int code() const noexcept { return mCode; }
        [[nodiscard]] sm::ZStringView message() const noexcept { return mMessage; }
        [[nodiscard]] const char *what() const noexcept { return mMessage.c_str(); }
        [[nodiscard]] const std::stacktrace& stacktrace() const noexcept { return mStacktrace; }

        operator bool() const noexcept { return mCode != eOk; }
        [[nodiscard]] bool isSuccess() const noexcept { return mCode == eOk; }

        template<IsException T>
        CT_NORETURN raise() const throws(T) {
            throw T{ *this };
        }

        template<IsException T>
        void throwIfFailed() const throws(T) {
            if (!isSuccess()) {
                raise<T>();
            }
        }
    };

    template<IsError T>
    class AnyException : public std::exception {
        T mError;

    public:
        [[nodiscard]] const char *what() const noexcept override { return mError.what(); }
        [[nodiscard]] const std::stacktrace& stacktrace() const noexcept { return mError.stacktrace(); }
    };

    template<typename T, IsError E>
    using AnyResult = std::expected<T, E>;
}
