#pragma once

#include <pthread.h>

#include <stdint.h>

namespace sm::system::os {
    constexpr int16_t kHighPriorityClass = -20;
    constexpr int16_t kNormalPriorityClass = 0;
    constexpr int16_t kIdlePriorityClass = 15;

    using Thread = pthread_t;
    using ThreadId = pthread_t;
    using StartRoutine = void *(*)(void *);

    constexpr Thread kInvalidThread = 0;

    inline void destroyThread(Thread thread) {
        joinThread(thread);
    }

    inline os_error_t joinThread(Thread thread) {
        return pthread_join(thread, nullptr);
    }
}
