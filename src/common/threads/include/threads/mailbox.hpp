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
        std::atomic<int> mReadIndex;
        std::atomic<bool> mShouldSwap;
        std::atomic<bool> mReadLock;
        T mSlots[2];

    public:
        NonBlockingMailBox() = default;

        void lock() {
            mReadLock.store(true);

            bool expected = true;
            if (mShouldSwap.compare_exchange_strong(expected, false)) { // update-0
                mReadIndex.fetch_xor(1); // update-1
            }
        }

        void unlock() {
            mReadLock.store(false);
        }

        const T& read() {
            CTASSERTF(mReadLock.load(), "Reading from mailbox without acquiring guard");
            // read-0
            return mSlots[mReadIndex.load()];
        }

        void write(T&& data) {
            int index = !mReadIndex.load();
            // update-1
            // read-0
            mSlots[index] = std::move(data);
            mShouldSwap.store(true);
        }
    };
}
