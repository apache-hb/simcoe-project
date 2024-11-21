#include "system/win32/network.hpp"

using namespace sm::system;

namespace chrono = std::chrono;

os::NetErrorCode os::destroySocket(os::SocketHandle socket) {
    return ::closesocket(socket);
}

static bool isSocketReady(os::SocketHandle socket) {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socket, &writefds);

    timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
    int result = ::select(0, nullptr, &writefds, nullptr, &timeout);
    return result > 0;
}

bool os::connectWithTimeout(os::SocketHandle socket, const sockaddr *addr, socklen_t len, std::chrono::milliseconds timeout) {
    const chrono::time_point start = chrono::steady_clock::now();
    auto timeoutReached = [&] { return start + timeout < chrono::steady_clock::now(); };

    while (!timeoutReached()) {
        int err = ::connect(socket, addr, len);
        if (err == os::kSuccess)
            return true;

        if (isSocketReady(socket))
            return true;
    }

    return false;
}
