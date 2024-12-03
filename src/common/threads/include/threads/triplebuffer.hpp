#pragma once

#include <atomic>
#include <new>
#include <bit>

namespace sm::threads {
    /// @brief single slot buffer for communicating between threads.
    /// single producer single consumer buffer which garantees to never
    /// block when reading or writing. Stores 3 copies of its template
    /// parameter internally to ensure this.
    /// @warning Can consume alot of memory, consider using a mutex
    ///          instead when T is large.
    template<typename T>
    class TripleBuffer {
        // clang bug maybe - see mailbox.hpp
        // alignas(std::hardware_destructive_interference_size)
        std::atomic<int> mState = 0;

        T mBuffers[3];

        // TODO: doesnt work yet

        int mReadIndex = 0;

        int claimRead() noexcept [[clang::nonblocking]] {
            int value = mState.fetch_add(1);
            return (value % 3);
        }

        int claimWrite() noexcept [[clang::nonblocking]] {
            int value = mState.fetch_add(1);
            return (value % 3);
        }

    public:
        TripleBuffer() = default;

        void lock() noexcept [[clang::nonblocking]] {

        }

        void unlock() noexcept [[clang::nonblocking]] {
            mState.fetch_sub(mReadIndex);
        }

        const T& read() noexcept [[clang::nonblocking]] {
            mReadIndex = claimRead();
            return mBuffers[mReadIndex];
        }

        void write(T data) noexcept {
            int value = mState.fetch_add(1);
            T *write = &mBuffers[(value % 3)];
            *write = data;
            mState.fetch_sub(value);
        }
    };
}
