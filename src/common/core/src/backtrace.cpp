#include "core/backtrace.hpp"

using namespace sm;

void ISystemError::wrap_begin(size_t error, void *user) {
    static_cast<ISystemError*>(user)->error_begin(error);
}

void ISystemError::wrap_frame(const bt_frame_t *frame, void *user) {
    static_cast<ISystemError*>(user)->error_frame(frame);
}

void ISystemError::wrap_end(void *user) {
    static_cast<ISystemError*>(user)->error_end();
}
