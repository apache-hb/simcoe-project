#include "system/posix/network.hpp"

#include <poll.h>

#include <fmtlib/format.h>

using namespace sm::system;

namespace chrono = std::chrono;

static void flushSocket(int socket) {
    char buffer[1024];
    while (::recv(socket, buffer, sizeof(buffer), 0) > 0);
}

static int clearSocketError(int socket) {
    int error = 1;
    socklen_t len = sizeof(error);
    if (::getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &len) == -1) {
        return -1;
    }

    return error;
}

os::NetErrorCode os::destroySocket(SocketHandle socket) {
    if (::shutdown(socket, SHUT_RDWR) == -1) {
        if (errno != ENOTCONN && errno != EINVAL) {
            return lastNetError();
        }
    }

    flushSocket(socket);

    clearSocketError(socket);

    if (::close(socket) == -1) {
        return lastNetError();
    }

    return kSuccess;
}

// mostly pulled from https://stackoverflow.com/a/61960339
// with timespec usage replaced with chrono
bool os::connectWithTimeout(os::SocketHandle socket, const sockaddr *addr, socklen_t len, std::chrono::milliseconds timeout) {
    if (::connect(socket, addr, len) == 0)
        return true;

    if (errno != EWOULDBLOCK && errno != EINPROGRESS)
        return false;

    const chrono::time_point start = chrono::steady_clock::now();
    chrono::time_point deadline = start + timeout;
    auto timeoutReached = [&] { return deadline < chrono::steady_clock::now(); };

    int rc = -1;

    do {
        if (timeoutReached())
            return false;

        struct pollfd pfds[] = {
            { .fd = socket, .events = POLLOUT }
        };

        rc = poll(pfds, std::size(pfds), timeout.count());
        if (rc > 0) {
            int error = 0;
            socklen_t len = sizeof(error);
            int ret = getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &len);
            if (ret == 0) errno = error;
            else rc = -1;
        }
    } while (rc == -1 && errno == EINTR);

    if (rc == 0) {
        errno = ETIMEDOUT;
        return false;
    }

    return true;
}
