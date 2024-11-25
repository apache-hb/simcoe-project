#pragma once

#include <atomic>
#include <new>

namespace sm::threads {
    /// @brief single slot buffer for communicating between threads.
    /// single producer single consumer buffer which garantees to never
    /// block when reading or writing. Stores 3 copies of its template
    /// parameter internally to ensure this.
    /// @warning Can consume alot of memory, consider using a mutex
    ///          instead when T is large.
    template<typename T>
    class NonBlockingTripleBuffer {
        alignas(std::hardware_destructive_interference_size)
        std::atomic<int> mState = 0;

        T mBuffers[3];

    public:
        NonBlockingTripleBuffer() = default;

        void lock();
        void unlock();

        const T& read();

        void write(T data);
    };
}
