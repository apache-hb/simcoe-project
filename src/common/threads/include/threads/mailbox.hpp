#pragma once

#include <atomic>
#include <new>

namespace sm::threads {
    /// @brief single slot mailbox for communicating between threads.
    /// single producer single consumer mailbox which garantees to never
    /// block when reading. Useful for communicating between a worker thread
    /// that only writes rarely and a main thread that reads frequently.
    /// @warning can block when writing
    template<typename T>
    class NonBlockingMailBox {
        static constexpr int kIndexBit = 0b0001;
        static constexpr int kWriteBit = 0b0010;

        // clang bug maybe, on linux adding this alignas causes clang to
        // generate all kinds of strange layout info for this class.
        // on windows it works fine, the performance difference is well into
        // placebo territory so i'll leave it out for now.
        // alignas(std::hardware_destructive_interference_size)
        std::atomic<int> mState = 0;

        T mSlots[2];

    public:
        NonBlockingMailBox() = default;

        void lock() noexcept [[clang::nonblocking]] {

        }

        void unlock() noexcept [[clang::nonblocking]] {
            mState.fetch_xor(kWriteBit, std::memory_order_release);
        }

        const T& read() noexcept [[clang::nonblocking]] {
            return mSlots[!(mState.load(std::memory_order_acquire) & kIndexBit)];
        }

        void write(const T& data) [[clang::blocking]] {
            int state = 0;
            while ((state = mState.load(std::memory_order_acquire)) & kWriteBit) {
                /* spin */
            }

            int newIndex = (state & kIndexBit);
            mSlots[newIndex] = data;

            mState.store(state ^ (kIndexBit | kWriteBit), std::memory_order_release);
        }
    };
}
