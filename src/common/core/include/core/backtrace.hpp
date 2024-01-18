#pragma once

#include <sm_core_api.hpp>

#include "backtrace/backtrace.h"

namespace sm {
    class SM_CORE_API ISystemError : public bt_error_t {
        void init(void);

        static void wrap_begin(size_t error, void *user);
        static void wrap_frame(const bt_frame_t *frame, void *user);
        static void wrap_end(void *user);

    protected:
        virtual void error_begin(size_t error) = 0;
        virtual void error_frame(const bt_frame_t *frame) = 0;
        virtual void error_end(void) = 0;

        ISystemError();
    };
}
