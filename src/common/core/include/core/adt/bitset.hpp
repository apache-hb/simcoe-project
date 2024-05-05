#pragma once

#include "core/unique.hpp"

#include <atomic>
#include <bit>
#include <algorithm>

namespace sm {
    namespace detail {
        template<typename T>
        class BitSetBase {
            sm::UniquePtr<T[]> mStorage;
            size_t mBitCount;

            constexpr void verifyIndex(size_t bit) const noexcept {
                CTASSERTF(bit < getBitCapacity(), "bit (%zu) must be less than number of bits available (%zu)", bit, getBitCapacity());
            }

            constexpr void zeroRange(size_t words) noexcept {
                std::fill_n(mStorage.get(), words, T(0));
            }

            constexpr void init(size_t bitcount) noexcept {
                CTASSERT(bitcount > 0);

                size_t capacity = computeWordCapacity(bitcount);

                // dont use updateStorage here, as we zero initialize the storage
                mStorage.reset(new T[capacity](), capacity);
                mBitCount = bitcount;
            }

            constexpr void resizeStorage(size_t bitcount) noexcept {
                CTASSERT(bitcount > 0);

                size_t currentCapacity = getWordCapacity();
                size_t newCapacity = computeWordCapacity(bitcount);

                if (newCapacity > currentCapacity) {
                    UniquePtr newStorage = sm::makeUnique<T[]>(newCapacity);
                    std::uninitialized_copy_n(mStorage.get(), currentCapacity, newStorage.get());
                    std::uninitialized_value_construct_n(newStorage.get() + currentCapacity, newCapacity - currentCapacity);
                    mStorage = std::move(newStorage);
                } else if (newCapacity < currentCapacity) {
                    UniquePtr newStorage = sm::makeUnique<T[]>(newCapacity);
                    std::uninitialized_copy_n(mStorage.get(), newCapacity, newStorage.get());
                    mStorage = std::move(newStorage);
                }

                mBitCount = bitcount;
            }

            constexpr void zeroStorage(size_t bitcount) noexcept {
                CTASSERT(bitcount > 0);

                size_t currentCapacity = getWordCapacity();
                size_t newCapacity = computeWordCapacity(bitcount);

                if (newCapacity > currentCapacity) {
                    mStorage.reset(new T[newCapacity](), newCapacity);
                } else {
                    zeroRange(newCapacity);
                }

                mBitCount = bitcount;
            }

        protected:
            static constexpr size_t computeWordCapacity(size_t bitcount) noexcept {
                return bitcount / kBitsPerWord + 1;
            }

            static constexpr size_t computeWordIndex(size_t bit) noexcept {
                return bit / kBitsPerWord;
            }

            static constexpr size_t computeBitMask(size_t bit) noexcept {
                return T(1) << (bit % kBitsPerWord);
            }

            constexpr auto& getWord(this auto& self, size_t bit) noexcept {
                self.verifyIndex(bit);
                return self.mStorage[computeWordIndex(bit)];
            }

            constexpr BitSetBase(size_t bitcount) noexcept {
                init(bitcount);
            }

        public:
            static constexpr inline size_t kBitsPerWord = sizeof(T) * CHAR_BIT;

            constexpr size_t getWordCapacity() const noexcept {
                return computeWordCapacity(mBitCount);
            }

            constexpr size_t getBitCapacity() const noexcept {
                return mBitCount;
            }

            /// @brief resize a bitset
            /// @note may not shrink the bitsets backing storage
            /// @param bitcount the new bit count
            constexpr void resize(size_t bitcount) noexcept {
                resizeStorage(bitcount);
            }

            constexpr void resizeAndClear(size_t bitcount) noexcept {
                zeroStorage(bitcount);
            }

            constexpr bool test(size_t bit) const noexcept {
                return getWord(bit) & computeBitMask(bit);
            }

            constexpr void set(size_t bit) noexcept {
                getWord(bit) |= computeBitMask(bit);
            }

            constexpr void clear(size_t bit) noexcept {
                getWord(bit) &= ~computeBitMask(bit);
            }

            constexpr void flip(size_t bit) noexcept {
                getWord(bit) ^= computeBitMask(bit);
            }

            constexpr void update(size_t bit, bool value) noexcept {
                if (value) {
                    set(bit);
                } else {
                    clear(bit);
                }
            }

            constexpr void clear() noexcept {
                zeroRange(getWordCapacity());
            }

            constexpr size_t popcount() const noexcept {
                size_t count = 0;
                for (size_t i = 0; i < getWordCapacity(); i++) {
                    count += std::popcount(mStorage[i]);
                }
                return count;
            }

            constexpr size_t freecount() const noexcept {
                return getBitCapacity() - popcount();
            }
        };
    }

    class BitSet final
        : public detail::BitSetBase<std::uint64_t>
    {
        using Super = detail::BitSetBase<std::uint64_t>;
    public:
        using Super::BitSetBase;

        constexpr BitSet(size_t bitcount) noexcept
            : Super(bitcount)
        { }

        /// @brief test and set a bit
        /// @param bit the bit to test and set
        /// @return true if the bit was already set, false otherwise
        constexpr bool testAndSet(size_t bit) noexcept {
            bool result = test(bit);
            set(bit);
            return result;
        }

        /// @brief test and exchange a bit with a new value
        /// @param bit the bit to test and exchange
        /// @param value the new value to set
        /// @return the previous value of the bit
        constexpr bool exchange(size_t bit, bool value) noexcept {
            bool result = test(bit);
            update(bit, value);
            return result;
        }

        constexpr void setRange(size_t first, size_t last) noexcept {
            for (size_t i = first; i < last; i++) {
                set(i);
            }
        }

        constexpr void clearRange(size_t first, size_t last) noexcept {
            for (size_t i = first; i < last; i++) {
                clear(i);
            }
        }
    };

    class AtomicBitSet final
        : public detail::BitSetBase<std::atomic_uint64_t>
    {
        using Super = detail::BitSetBase<std::atomic_uint64_t>;

        static constexpr void verifyRangeIsAligned(size_t first, size_t last) noexcept {
            CTASSERTF(first / kBitsPerWord == last / kBitsPerWord, "range must not cross a word boundary");
        }
    public:
        using Super::BitSetBase;

        constexpr AtomicBitSet(size_t bitcount) noexcept
            : Super(bitcount)
        { }

        /// @brief test and set a bit
        /// @param bit the bit to test and set
        /// @return true if the bit was already set, false otherwise
        constexpr bool testAndSet(size_t bit) noexcept {
            uint64_t mask = computeBitMask(bit);

            return !(getWord(bit).fetch_or(mask) & mask);
        }

        /// @brief test and exchange a bit with a new value
        /// @param bit the bit to test and exchange
        /// @param value the new value to set
        /// @return the previous value of the bit
        constexpr bool exchange(size_t bit, bool value) noexcept {
            uint64_t mask = computeBitMask(bit);

            if (value) {
                return !(getWord(bit).fetch_or(mask) & mask);
            } else {
                return (getWord(bit).fetch_and(~mask) & mask);
            }
        }

        /// @brief set a range of bits
        /// @note the range specified by [first, last) must not cross a word boundary
        /// @param first the first bit to set
        /// @param last the last bit to set
        constexpr void setRange(size_t first, size_t last) noexcept {
            verifyRangeIsAligned(first, last);

            uint64_t mask = (uint64_t(1) << (last - first)) - 1;
            getWord(first).fetch_or(mask << (first % kBitsPerWord));
        }

        /// @brief clear a range of bits
        /// @note the range specified by [first, last) must not cross a word boundary
        /// @param first the first bit to clear
        /// @param last the last bit to clear
        constexpr void clearRange(size_t first, size_t last) noexcept {
            verifyRangeIsAligned(first, last);

            uint64_t mask = (uint64_t(1) << (last - first)) - 1;
            getWord(first).fetch_and(~(mask << (first % kBitsPerWord)));
        }
    };
}