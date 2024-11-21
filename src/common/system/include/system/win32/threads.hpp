#pragma once

#include "core/win32.hpp" // IWYU pragma: keep

#include <stdint.h>

namespace sm::system::os {
    constexpr int16_t kHighPriorityClass = HIGH_PRIORITY_CLASS;
    constexpr int16_t kNormalPriorityClass = NORMAL_PRIORITY_CLASS;
    constexpr int16_t kIdlePriorityClass = THREAD_PRIORITY_IDLE;

    using Thread = HANDLE;
    using ThreadId = DWORD;
    using StartRoutine = LPTHREAD_START_ROUTINE;
}
