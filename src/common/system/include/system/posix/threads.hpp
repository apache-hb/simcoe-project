#pragma once

#include "os/core.h"
#include <pthread.h>

#include <stdint.h>

#define SM_OS_CREATE_THREAD "pthread_create"

namespace sm::system::os {
    constexpr int16_t kHighPriorityClass = -20;
    constexpr int16_t kNormalPriorityClass = 0;
    constexpr int16_t kIdlePriorityClass = 15;

    using Thread = pthread_t;
    using ThreadId = pthread_t;
    using StartRoutine = void *(*)(void *);

    constexpr Thread kInvalidThread = 0;

    inline os_error_t joinThread(Thread thread) {
        return pthread_join(thread, nullptr);
    }

    inline void destroyThread(Thread thread) {
        joinThread(thread);
    }

    inline os_error_t createThread(Thread *thread, ThreadId *threadId, StartRoutine start, void *param) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        os_error_t err = pthread_create(thread, &attr, start, param);
        if (err == 0) {
            *threadId = *thread;
        }

        pthread_attr_destroy(&attr);
        return err;
    }
}
