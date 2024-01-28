#pragma once

#include "backtrace/backtrace.h"

namespace sm {
    class ISystemError : public bt_error_t {
        constexpr void init(void) {
            begin = wrap_begin;
            next = wrap_frame;
            end = wrap_end;
            user = this;
        }

        static void wrap_begin(size_t error, void *user);
        static void wrap_frame(const bt_frame_t *frame, void *user);
        static void wrap_end(void *user);

    protected:
        virtual void error_begin(size_t error) = 0;
        virtual void error_frame(const bt_frame_t *frame) = 0;
        virtual void error_end(void) = 0;

        constexpr ISystemError() {
            init();
        }
    };
}
