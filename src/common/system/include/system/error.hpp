#pragma once

#include "core/error/error.hpp"

#include "os/core.h"

// TODO: posix error handling, currently on windows we use OsError even for posix exceptions
// which is not ideal, it means we log the wrong error messages when stuff fails.

namespace sm {
    class PosixError : public errors::Error<PosixError> {
        using Super = errors::Error<PosixError>;
        int mError;

    public:
        using Exception = class PosixException;

        PosixError(int error);
        PosixError(int error, std::string_view message);

        template<typename... A> requires (sizeof...(A) > 0)
        PosixError(int error, fmt::format_string<A...> fmt, A&&... args)
            : PosixError(error, fmt::vformat(fmt, fmt::make_format_args(args...)))
        { }

        constexpr int error() const noexcept { return mError; }
        constexpr bool isSuccess() const noexcept { return mError == 0; }
    };

    class PosixException : public errors::Exception<PosixError> {
        using Super = errors::Exception<PosixError>;

    public:
        using Super::Super;

        template<typename... A>
        PosixException(int error, fmt::format_string<A...> fmt, A&&... args)
            : Super(PosixError(error, fmt::vformat(fmt, fmt::make_format_args(args...))))
        { }
    };

    class OsError : public errors::Error<OsError> {
        using Super = errors::Error<OsError>;
        os_error_t mError;

    public:
        using Exception = class OsException;

        OsError(os_error_t error);
        OsError(os_error_t error, std::string_view message);

        template<typename... A> requires (sizeof...(A) > 0)
        OsError(os_error_t error, fmt::format_string<A...> fmt, A&&... args)
            : OsError(error, fmt::vformat(fmt, fmt::make_format_args(args...)))
        { }

        constexpr os_error_t error() const noexcept { return mError; }
        constexpr bool isSuccess() const noexcept { return mError == 0; }
    };

    class OsException : public errors::Exception<OsError> {
        using Super = errors::Exception<OsError>;

    public:
        using Super::Super;

        template<typename... A>
        OsException(os_error_t error, fmt::format_string<A...> fmt, A&&... args)
            : Super(OsError(error, fmt::vformat(fmt, fmt::make_format_args(args...))))
        { }
    };
}
