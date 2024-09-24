#include "stdafx.hpp"

#include "core/error.hpp"

using namespace sm;

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

CT_NORETURN OsError::raise() const noexcept(false) {
    throw OsException(*this);
}

void OsError::throwIfFailed() const noexcept(false) {
    if (failed()) raise();
}
