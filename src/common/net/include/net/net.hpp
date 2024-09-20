#pragma once

#include "core/macros.hpp"
#include "core/throws.hpp"

#include <expected>
#include <string>

#include <WinSock2.h>

namespace sm::net {
    class NetError {
        int mCode;
        std::string mMessage;

    public:
        NetError(int code) noexcept;

        int code() const noexcept { return mCode; }
        std::string_view message() const noexcept { return mMessage; }
        const char *c_str() const noexcept { return mMessage.c_str(); }

        [[noreturn]]
        void raise() const throws(NetException);

        void throwIfFailed() const throws(NetException);

        operator bool() const noexcept { return mCode != 0; }

        bool cancelled() const noexcept { return mCode == WSAEINTR; }
    };

    class NetException : public std::exception {
        NetError mError;

    public:
        NetException(NetError error) noexcept
            : mError(std::move(error))
        { }

        const char *what() const noexcept override { return mError.c_str(); }
    };

    template<typename T>
    using NetResult = std::expected<T, NetError>;

    template<typename T>
    T throwIfFailed(NetResult<T> result) throws(NetException) {
        if (result)
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
            return IPv4Address(127, 0, 0, 1);
        }
    };

    std::string toString(const IPv4Address& addr) noexcept;

    class Socket {
    protected:
        void closeSocket() noexcept;

        SOCKET mSocket = INVALID_SOCKET;

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
