#pragma once

#include <atomic>

namespace sm::threads {
    /**
     * @brief Single producer single consumer non-blocking mailbox.
     *
     * Uses two slots to ensure that the reader never blocks.
     * The writer may block if it tries to write while a read is in progress.
     * Requires T to be default initializable and either noexcept move constructible
     * or noexcept copy constructible.
     *
     * @code{.cpp}
     * NonBlockingMailBox<MyData> mailbox;
     *
     * // Writer thread
     * {
     *     mailbox.write(MyData{...});
     * }
     *
     * // Reader thread
     * {
     *     std::lock_guard guard(mailbox);
     *     const MyData& data = mailbox.read();
     *     // process data...
     * }
     * @endcode
     *
     * @tparam T The type of data to be communicated.
     */
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

        /**
         * @brief Locks the mailbox for reading.
         */
        void lock() noexcept [[clang::nonblocking]] {

        }

        /**
         * @brief Unlocks the mailbox after reading.
         */
        void unlock() noexcept [[clang::nonblocking]] {
            mState.fetch_xor(kWriteBit, std::memory_order_release);
        }

        /**
         * @brief Reads the latest data from the mailbox.
         * @note Requires the mailbox to be locked.
         *
         * @return The most recent data.
         */
        const T& read() noexcept [[clang::nonblocking]] {
            size_t index = !(mState.load(std::memory_order_acquire) & kIndexBit);
            return mSlots[index];
        }

        /**
         * @brief Writes data to the mailbox.
         *
         * @param data The data to write.
         */
        void write(T data) [[clang::blocking]] {
            int state = 0;
            while ((state = mState.load(std::memory_order_acquire)) & kWriteBit) {
                /* spin */
            }

            int newIndex = (state & kIndexBit);

            mSlots[newIndex] = std::move(data);

            mState.store(state ^ (kIndexBit | kWriteBit), std::memory_order_release);
        }
    };

    template<typename T>
    using WaitFreeMailBox = NonBlockingMailBox<T>;
}
