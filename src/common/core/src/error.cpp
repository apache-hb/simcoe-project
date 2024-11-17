#include "stdafx.hpp"

#include "core/error.hpp"

using namespace sm;

static std::string osErrorToString(os_error_t error) {
    char buffer[512];
    size_t size = os_error_get_string(error, buffer, sizeof(buffer));
    buffer[std::min(size, sizeof(buffer) - 1)] = '\0';
    return buffer;
}

void ISystemError::wrap_begin(size_t error, void *user) {
    static_cast<ISystemError*>(user)->error_begin(error);
}

void ISystemError::wrap_frame(bt_address_t frame, void *user) {
    static_cast<ISystemError*>(user)->error_frame(frame);
}

void ISystemError::wrap_end(void *user) {
    static_cast<ISystemError*>(user)->error_end();
}

void sm::panic(source_info_t info, std::string_view msg) {
    ctu_panic(info, "%.*s", (int)msg.size(), msg.data());
}

OsError::OsError(os_error_t error, std::string_view message)
    : Super(fmt::format("{} ({})", message, osErrorToString(error)))
    , mError(error)
{ }

OsError::OsError(os_error_t error)
    : Super(osErrorToString(error))
    , mError(error)
{ }
