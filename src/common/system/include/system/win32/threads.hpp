#pragma once

#include "core/win32.hpp" // IWYU pragma: keep

#include "os/core.h"

#include <stdint.h>

#define SM_OS_CREATE_THREAD "CreateThread"

namespace sm::system::os {
    constexpr int16_t kHighPriorityClass = HIGH_PRIORITY_CLASS;
    constexpr int16_t kNormalPriorityClass = NORMAL_PRIORITY_CLASS;
    constexpr int16_t kIdlePriorityClass = THREAD_PRIORITY_IDLE;

    using Thread = HANDLE;
    using ThreadId = DWORD;
    using StartRoutine = LPTHREAD_START_ROUTINE;

    constexpr Thread kInvalidThread = nullptr;

    inline os_error_t createThread(Thread *thread, ThreadId *threadId, StartRoutine start, void *param) {
        if (Thread handle = ::CreateThread(nullptr, 0, start, param, 0, threadId)) {
            *thread = handle;
            return ERROR_SUCCESS;
        } else {
            return ::GetLastError();
        }
    }

    inline void destroyThread(Thread thread) {
        ::CloseHandle(thread);
    }

    inline os_error_t joinThread(Thread thread) {
        DWORD result = ::WaitForSingleObject(thread, INFINITE);
        switch (result) {
        case WAIT_OBJECT_0: return ERROR_SUCCESS;
        case WAIT_TIMEOUT: return ERROR_TIMEOUT;
        default: return ::GetLastError();
        }
    }
}
