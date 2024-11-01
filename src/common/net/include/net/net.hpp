#pragma once

#include "core/macros.hpp"
#include "core/throws.hpp"
#include "core/error/error.hpp"

#include <expected>
#include <span>
#include <string>
#include <chrono>
#include <array>

#include <fmtlib/format.h>

#include <WinSock2.h>
#include <ws2ipdef.h>

// 12000 to 12999 are available for use by applications
#define SNET_FIRST_STATUS 12000

#define SNET_END_OF_PACKET 12001
#define SNET_READ_TIMEOUT 12002
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

        bool cancelled() const noexcept { return mCode == WSAEINTR; }
        bool timeout() const noexcept { return mCode == SNET_READ_TIMEOUT; }
        bool connectionClosed() const noexcept { return mCode == SNET_CONNECTION_CLOSED; }

        static NetError ok() noexcept { return NetError{0}; }
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

    enum class AddressType {
        eIPv4,
        eIPv6,
    };

    class Address {
    public:
        static constexpr inline size_t kIpv4Size = 4;

        using IPv4Bytes = std::span<const uint8_t, kIpv4Size>;
        using IPv4Data = std::array<uint8_t, kIpv4Size>;

        static constexpr Address ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept {
            return IPv4Data {a, b, c, d};
        }

        IPv4Bytes v4address() const noexcept { return mIpv4Address; }
        bool hasHostName() const noexcept { return !mHostName.empty(); }
        std::string hostName() const noexcept { return mHostName; }

        static constexpr Address loopback() noexcept {
            return ipv4(127, 0, 0, 1);
        }

        static constexpr Address any() noexcept {
            return ipv4(0, 0, 0, 0);
        }

        static constexpr Address of(std::string host) noexcept {
            return Address{std::move(host)};
        }

    private:
        IPv4Data mIpv4Address;
        std::string mHostName;

        constexpr Address(IPv4Data data) noexcept
            : mIpv4Address(data)
        { }

        constexpr Address(std::string host) noexcept
            : mHostName(std::move(host))
        { }
    };

    std::string toAddressString(const Address& addr);
    std::string toString(const Address& addr);

    struct [[nodiscard]] ReadResult {
        size_t size;
        NetError error;
    };

    class Socket {
    protected:
        void closeSocket() noexcept;

        SOCKET mSocket = INVALID_SOCKET;
        bool mBlocking = true;

    public:
        Socket(SOCKET socket) noexcept
            : mSocket(socket)
        { }

        Socket(Socket&& other) noexcept
            : mSocket(std::exchange(other.mSocket, INVALID_SOCKET))
        { }

        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                closeSocket();
                mSocket = std::exchange(other.mSocket, INVALID_SOCKET);
            }

            return *this;
        }

        ~Socket() noexcept;

        SM_NOCOPY(Socket);

        NetResult<size_t> sendBytes(const void *data, size_t size) noexcept;
        NetResult<size_t> recvBytes(void *data, size_t size) noexcept;

        ReadResult recvBytesTimeout(void *data, size_t size, std::chrono::milliseconds timeout) noexcept;

        template<typename T> requires (std::is_standard_layout_v<T>)
        NetResult<T> recv() noexcept {
            T value;
            size_t result = TRY_RESULT(recvBytes(&value, sizeof(T)));

            if (result != sizeof(T))
                return std::unexpected{NetError(SNET_END_OF_PACKET, "expected {} bytes, received {}", sizeof(T), result)};

            return value;
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        NetResult<T> recvTimed(std::chrono::milliseconds timeout) noexcept {
            T value;
            ReadResult result = recvBytesTimeout(&value, sizeof(T), timeout);

            if (result.size != sizeof(T))
                return std::unexpected{NetError(SNET_END_OF_PACKET, "expected {} bytes, received {}", sizeof(T), result.size)};

            return value;
        }

        template<typename T> requires (std::is_standard_layout_v<T>)
        NetError send(const T& value) noexcept {
            size_t result = TRY_UNWRAP(sendBytes(&value, sizeof(T)));

            if (result != sizeof(T))
                return NetError(SNET_END_OF_PACKET, "expected {} bytes, sent {}", sizeof(T), result);

            return NetError::ok();
        }

        NetError setBlocking(bool blocking) noexcept;
        bool isBlocking() const noexcept { return mBlocking; }
        bool isActive() const noexcept { return mSocket != INVALID_SOCKET; }

        NetError setRecvTimeout(std::chrono::milliseconds timeout) noexcept;
        NetError setSendTimeout(std::chrono::milliseconds timeout) noexcept;
    };

    class ListenSocket : public Socket {
    public:
        SM_NOCOPY(ListenSocket);
        SM_MOVE(ListenSocket, default);

        ~ListenSocket() noexcept;

        using Socket::Socket;

        static constexpr int kMaxBacklog = SOMAXCONN;

        NetResult<Socket> tryAccept() noexcept;
        Socket accept() throws(NetException) {
            return throwIfFailed(tryAccept());
        }

        void cancel() noexcept;

        NetError listen(int backlog) noexcept;
    };

    class Network {
        Network() = default;

    public:
        static Network create() throws(NetException);

        Socket connect(const Address& address, uint16_t port) throws(NetException);
        Socket connectWithTimeout(const Address& address, uint16_t port, std::chrono::milliseconds timeout) throws(NetException);

        ListenSocket bind(const Address& address, uint16_t port) throws(NetException);
    };

    void create(void);
    void destroy(void) noexcept;
}

template<>
struct fmt::formatter<sm::net::NetError> {
    constexpr auto parse(format_parse_context& ctx) const {
        return ctx.begin();
    }

    constexpr auto format(const sm::net::NetError& error, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "NetError({}: {})", error.code(), error.message());
    }
};
