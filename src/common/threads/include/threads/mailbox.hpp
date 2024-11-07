#pragma once

#include <atomic>
#include <mutex>

#include "base/panic.h"

namespace sm::thread {
    /// @brief single slot mailbox for communicating between threads
    /// single producer single consumer mailbox which garantees to never
    /// block when reading.
    /// @note can block when writing
    template<typename T>
    class NonBlockingMailBox {
        static constexpr size_t kIndexBit = 0b0001;
        static constexpr size_t kWriteBit = 0b0010;

        alignas(std::hardware_destructive_interference_size)
        std::atomic<int> mState = 0;

        T mSlots[2];

    public:
        NonBlockingMailBox() = default;

        void lock() {

        }

        void unlock() {
            mState ^= kWriteBit;
        }

        const T& read() {
            return mSlots[!(mState.load(std::memory_order_relaxed) & kIndexBit)];
        }

        void write(T data) {
            int state = 0;
            while ((state = mState.load(std::memory_order_acquire)) & kWriteBit) {
                /* spin */
            }

            int newIndex = (state & kIndexBit);
            mSlots[newIndex] = std::move(data);

            mState.store(state ^ (kIndexBit | kWriteBit), std::memory_order_release);
        }
    };
}
