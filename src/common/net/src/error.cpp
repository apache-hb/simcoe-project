#include "stdafx.hpp"

#include "core/error.hpp"
#include "net/error.hpp"

using namespace sm;
using namespace sm::net;

static std::string fmtOsError(int code) {
    switch (code) {
    case SNET_END_OF_PACKET:
        return "End of packet (" CT_STR(SNET_END_OF_PACKET) ")";
    case SNET_CONNECTION_CLOSED:
        return "Connection closed (" CT_STR(SNET_CONNECTION_CLOSED) ")";
    case SNET_CONNECTION_FAILED:
        return "Connection failed (" CT_STR(SNET_CONNECTION_FAILED) ")";
    default:
        return fmt::to_string(OsError(code));
    }
}

NetError::NetError(int code, std::string_view message)
    : Super(fmt::format("{} ({})", message, fmtOsError(code)))
    , mCode(code)
{ }

NetError::NetError(int code)
    : Super(fmtOsError(code))
    , mCode(code)
{ }

bool NetError::cancelled() const noexcept {
    return mCode == system::os::kErrorInterrupted;
}

bool NetError::timeout() const noexcept {
    return mCode == system::os::kErrorTimeout;
}

bool NetError::connectionClosed() const noexcept {
    return mCode == SNET_CONNECTION_CLOSED;
}

NetError NetError::ok() noexcept {
    return NetError{system::os::kSuccess};
}
