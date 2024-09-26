#pragma once

#include "core/macros.hpp"
#include "core/throws.hpp"

#include <expected>
#include <string>
#include <chrono>

#include <fmtlib/format.h>

#include <WinSock2.h>

// 12000 to 12999 are available for use by applications
#define SNET_FIRST_STATUS 12000

#define SNET_END_OF_PACKET 12001
#define SNET_READ_TIMEOUT 12002
#define SNET_CONNECTION_CLOSED 12003

#define SNET_LAST_STATUS 12999

namespace sm::net {
    class NetError {
        int mCode;
        std::string mMessage;

    public:
        NetError(int code) noexcept;

        template<typename... A>
        NetError(int code, fmt::format_string<A...> fmt, A&&... args)
            : mCode(code)
            , mMessage(fmt::vformat(fmt, fmt::make_format_args(args...)))
        { }

        int code() const noexcept { return mCode; }
        std::string_view message() const noexcept { return mMessage; }
        const char *what() const noexcept { return mMessage.c_str(); }

        [[noreturn]]
        void raise() const throws(NetException);

        void throwIfFailed() const throws(NetException);

        operator bool() const noexcept { return mCode != 0; }
        bool isSuccess() const noexcept { return mCode == 0; }

        bool cancelled() const noexcept { return mCode == WSAEINTR; }
        bool timeout() const noexcept { return mCode == SNET_READ_TIMEOUT; }
        bool connectionClosed() const noexcept { return mCode == SNET_CONNECTION_CLOSED; }

        static NetError ok() noexcept { return NetError{0}; }
    };

    class NetException : public std::exception {
        NetError mError;

    public:
        NetException(NetError error) noexcept
            : mError(std::move(error))
        { }

        const char *what() const noexcept override { return mError.what(); }
    };

    template<typename T>
    using NetResult = std::expected<T, NetError>;

    template<typename T>
    T throwIfFailed(NetResult<T> result) throws(NetException) {
        if (result.has_value())
            return std::move(result.value());

        throw NetException(result.error());
    }

    class IPv4Address {
        uint8_t mAddress[4];

    public:
        constexpr IPv4Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept
            : mAddress(a, b, c, d)
        { }

        const uint8_t *address() const noexcept { return mAddress; }

        static constexpr IPv4Address loopback() noexcept {
            return { 127, 0, 0, 1 };
        }
    };

    std::string toString(const IPv4Address& addr) noexcept;

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
        NetError send(const T& value) noexcept {
            size_t result = TRY_UNWRAP(sendBytes(&value, sizeof(T)));

            if (result != sizeof(T))
                return NetError(SNET_END_OF_PACKET, "expected {} bytes, sent {}", sizeof(T), result);

            return NetError::ok();
        }

        NetError setBlocking(bool blocking) noexcept;
        bool isBlocking() const noexcept { return mBlocking; }

        NetError setRecvTimeout(std::chrono::milliseconds timeout) noexcept;
        NetError setSendTimeout(std::chrono::milliseconds timeout) noexcept;
    };

    class ListenSocket : public Socket {
    public:
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
        WSADATA mData;

        Network(WSADATA data) noexcept
            : mData(data)
        { }

        void cleanup() noexcept;

    public:
        ~Network() noexcept;

        SM_NOCOPY(Network);

        Network(Network&& other) noexcept
            : mData(std::exchange(other.mData, WSADATA{}))
        { }

        Network& operator=(Network&& other) noexcept = delete;

        static NetResult<Network> tryCreate() noexcept;
        static Network create() throws(NetException) {
            return throwIfFailed(tryCreate());
        }

        NetResult<Socket> tryConnect(IPv4Address address, uint16_t port) noexcept;
        Socket connect(IPv4Address address, uint16_t port) throws(NetException) {
            return throwIfFailed(tryConnect(address, port));
        }

        NetResult<ListenSocket> tryBind(IPv4Address address, uint16_t port) noexcept;
        ListenSocket bind(IPv4Address address, uint16_t port) throws(NetException) {
            return throwIfFailed(tryBind(address, port));
        }

        size_t getMaxSockets() const noexcept { return mData.iMaxSockets; }
    };
}
