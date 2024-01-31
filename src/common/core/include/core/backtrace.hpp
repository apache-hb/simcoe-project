#pragma once

#include "backtrace/backtrace.h"

#include "os/os.h"

namespace sm {
    class ISystemError : public bt_error_t {
        static void wrap_begin(size_t error, void *user);
        static void wrap_frame(const bt_frame_t *frame, void *user);
        static void wrap_end(void *user);

    protected:
        virtual void error_begin(os_error_t error) = 0;
        virtual void error_frame(const bt_frame_t *frame) = 0;
        virtual void error_end() = 0;

        constexpr ISystemError() {
            begin = wrap_begin;
            next = wrap_frame;
            end = wrap_end;
            user = this;
        }
    };
}
