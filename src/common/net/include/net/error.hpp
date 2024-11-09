#pragma once

#include "core/throws.hpp"
#include "core/error/error.hpp"

// 12000 to 12999 are available for use by applications
#define SNET_FIRST_STATUS 12000

#define SNET_END_OF_PACKET 12001
#define SNET_CONNECTION_CLOSED 12003
#define SNET_CONNECTION_FAILED 12004

#define SNET_LAST_STATUS 12999

namespace sm::net {
    class NetException;
    class NetError;

    class NetError : public errors::Error<NetError> {
        using Super = errors::Error<NetError>;

        int mCode;
    public:
        using Exception = NetException;

        NetError(int code);

        template<typename... A>
        NetError(int code, fmt::format_string<A...> fmt, A&&... args)
            : Super(fmt::vformat(fmt, fmt::make_format_args(args...)))
            , mCode(code)
        { }

        int code() const noexcept { return mCode; }
        bool isSuccess() const noexcept { return mCode == 0; }

        bool cancelled() const noexcept;
        bool timeout() const noexcept;
        bool connectionClosed() const noexcept;

        static NetError ok() noexcept;
    };

    class NetException : public errors::Exception<NetError> {
        using Super = errors::Exception<NetError>;
    public:
        using Super::Super;

        template<typename... A>
        NetException(int code, fmt::format_string<A...> fmt, A&&... args)
            : Super(NetError(code, fmt, args...))
        { }
    };

    template<typename T>
    using NetResult = NetError::Result<T>;

    template<typename T>
    bool isCancelled(const NetResult<T>& result) noexcept {
        return !result.has_value() && result.error().cancelled();
    }

    template<typename T>
    bool isConnectionClosed(const NetResult<T>& result) noexcept {
        return !result.has_value() && result.error().connectionClosed();
    }

    template<typename T>
    T throwIfFailed(NetResult<T> result) throws(NetException) {
        if (result.has_value())
            return std::move(result.value());

        throw NetException(result.error());
    }
}