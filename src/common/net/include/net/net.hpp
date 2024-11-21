#pragma once

#include "net/error.hpp"
#include "net/address.hpp"

#include "core/macros.hpp"
#include "core/throws.hpp"

#include "system/network.hpp"

#include <atomic>
#include <expected>
#include <chrono>

#include <fmtlib/format.h>

namespace sm::net {
    struct [[nodiscard]] ReadResult {
        size_t size;
        NetError error;
    };

    class Socket {
    protected:
        void closeSocket() noexcept;

        static constexpr int kBlockingFlag = (1 << 0);
        static constexpr int kShutdownFlag = (1 << 1);

        std::atomic<int> mFlags;

        system::os::SocketHandle mSocket = system::os::kInvalidSocket;

    public:
        Socket(system::os::SocketHandle socket) noexcept
            : mSocket(socket)
        { }

        /// @note not internally synchronized
        Socket(Socket&& other) noexcept
            : mSocket(std::exchange(other.mSocket, system::os::kInvalidSocket))
        { }

        /// @note not internally synchronized
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                closeSocket();
                mSocket = std::exchange(other.mSocket, system::os::kInvalidSocket);
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
        bool isBlocking() const noexcept { return mFlags.load(std::memory_order_seq_cst) & kBlockingFlag; }
        bool isActive() const noexcept { return !(mFlags.load(std::memory_order_seq_cst) & kShutdownFlag); }

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

        uint16_t getBoundPort();
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
