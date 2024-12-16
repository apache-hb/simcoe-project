#pragma once

#include "base/macros.hpp"
#include "core/win32.hpp"

namespace sm::threads {
    class Mutex {
        CRITICAL_SECTION mCriticalSection;

    public:
        Mutex() noexcept [[clang::blocking]];
        ~Mutex() noexcept [[clang::blocking]];

        SM_NOCOPY(Mutex);
        SM_NOMOVE(Mutex);

        void lock() noexcept [[clang::blocking]];
        bool try_lock() noexcept [[clang::nonblocking]];

        void unlock() noexcept [[clang::nonblocking]];
    };
}
