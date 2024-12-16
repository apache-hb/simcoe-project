#pragma once

#include "core/memory/unique.hpp"
#include "net/error.hpp"
#include "net/address.hpp"

#include "base/macros.hpp"
#include "base/throws.hpp"

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

    void destroyHandle(system::os::SocketHandle& handle);

    class Socket {
    protected:
        static constexpr int kBlockingFlag = (1 << 0);

        std::unique_ptr<std::atomic<system::os::SocketHandle>> mSocket;

        // TODO: cmon c++, please let me move a damn atomic
        std::unique_ptr<std::atomic<int>> mFlags = std::make_unique<std::atomic<int>>(0);

    public:
        Socket(system::os::SocketHandle socket) noexcept
            : mSocket(std::make_unique<std::atomic<system::os::SocketHandle>>(socket))
        { }

        ~Socket() noexcept;

        Socket(Socket&& other) noexcept
            : mSocket(std::move(other.mSocket))
            , mFlags(std::move(other.mFlags))
        { }

        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                mSocket = std::move(other.mSocket);
                mFlags = std::move(other.mFlags);
            }

            return *this;
        }

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
        bool isBlocking() const noexcept { return mFlags->load(std::memory_order_seq_cst) & kBlockingFlag; }
        bool isActive() const noexcept;

        NetError setRecvTimeout(std::chrono::milliseconds timeout) noexcept;
        NetError setSendTimeout(std::chrono::milliseconds timeout) noexcept;

        system::os::SocketHandle get() const noexcept {
            return mSocket ? mSocket->load() : system::os::kInvalidSocket;
        }
    };

    class ListenSocket : public Socket {
    public:
        using Socket::Socket;

        static constexpr int kMaxBacklog = SOMAXCONN;

        NetResult<Socket> tryAccept() noexcept;
        Socket accept() throws(NetException);

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
    bool isSetup(void) noexcept;
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
