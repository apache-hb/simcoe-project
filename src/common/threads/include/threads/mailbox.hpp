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
        static const uint32_t kIndexMask        = 0b0001;
        static const uint32_t kActiveReaderMask = 0b0010;
        static const uint32_t kNewDataMask      = 0b0100;

        // bit 0 is the last written slot
        // bit 1 is if the reader is currently active
        std::atomic<uint32_t> mState;

        int mReadIndex = 0;
        T mSlots[2];

    public:
        NonBlockingMailBox() = default;

        void lock() {
            mReadIndex = mState.fetch_or(kActiveReaderMask) & kIndexMask;
        }

        void unlock() {
            mState &= (kIndexMask | kNewDataMask);
        }

        const T& read() {
            return mSlots[mReadIndex];
        }

        void write(T&& data) {
            uint32_t state = mState.load();
        }
    };
}
