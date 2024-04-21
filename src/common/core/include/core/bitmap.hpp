#pragma once

#include "core/array.hpp"

#include <atomic>
#include <bit>

namespace sm {
    namespace detail {
        /// @brief bitmap backing storage
        /// @tparam T the type of the backing storage
        /// @tparam Super crtp superclass
        template<typename T, typename Super>
        struct BitSetStorage {
            enum Index : size_t { eInvalid = SIZE_MAX };
            using IndexType = __underlying_type(Index);

            constexpr BitSetStorage() = default;

            constexpr BitSetStorage(size_t bitcount) noexcept
                : mStorage(bitcount / kBitsPerWord + 1)
            { }

            constexpr void resize(size_t bitcount) noexcept {
                size_t words = bitcount / kBitsPerWord + 1;
                mStorage.resize(words);
                reset();
            }

            constexpr size_t popcount() const noexcept {
                size_t count = 0;
                for (auto word : mStorage) {
                    count += std::popcount(word);
                }
                return count;
            }

            constexpr size_t freecount() const noexcept {
                return get_capacity() - popcount();
            }

            constexpr Index scan_set_first(size_t limit) noexcept {
                Super *self = static_cast<Super*>(this);
                for (uint32_t i = 0; i < limit; i++) {
                    if (self->test_set(Index{i})) {
                        return Index{i};
                    }
                }

                return Index::eInvalid;
            }

            constexpr Index scan_set_first() noexcept {
                return scan_set_first(get_capacity());
            }

            constexpr Index allocate() noexcept {
                return scan_set_first();
            }

            constexpr size_t get_capacity() const noexcept { return mStorage.length() * kBitsPerWord; }
            constexpr bool is_valid() const noexcept { return mStorage.is_valid(); }

            constexpr void reset() noexcept {
                if (mStorage.is_valid()) mStorage.fill(0);
            }

            constexpr void release(Index index) noexcept {
                CTASSERTF(test(index), "index %zu is not set", std::to_underlying(index));
                clear(index);
            }

            constexpr static size_t kBitsPerWord = sizeof(T) * CHAR_BIT;

            constexpr bool test(Index index) const noexcept {
                verify_index(index);

                return mStorage[get_word(index)] & get_mask(index);
            }

            constexpr void set(Index index) noexcept {
                verify_index(index);
                mStorage[get_word(index)] |= get_mask(index);
            }

            constexpr void clear(Index index) noexcept {
                verify_index(index);
                mStorage[get_word(index)] &= ~get_mask(index);
            }

        protected:
            constexpr T get_mask(Index bit) const noexcept { return T(1) << (bit % kBitsPerWord); }
            constexpr size_t get_word(Index bit) const noexcept { return bit / kBitsPerWord; }

            sm::UniqueArray<T> mStorage;

            constexpr void verify_index(Index bit) const noexcept {
                CTASSERTF(bit != Index::eInvalid, "invalid index");
                CTASSERTF(bit <= get_capacity(), "bit %zu is out of bounds", bit);
            }
        };
    }

    struct BitMap final : detail::BitSetStorage<std::uint64_t, BitMap> {
        using Super = detail::BitSetStorage<std::uint64_t, BitMap>;
        using Super::BitSetStorage;

        constexpr void set_range(Index front, Index back) noexcept {
            verify_index(front);
            verify_index(back);

            for (size_t i = front; i <= back; i++) {
                set(Index(i)); // TODO: optimize
            }
        }

        constexpr bool test_range(Index front, Index back) noexcept {
            verify_index(front);
            verify_index(back);

            for (uint32_t i = front; i <= back; i++) {
                if (!test(Index{i})) {
                    return false;
                }
            }

            return true;
        }

        constexpr Index scan_set_range(Index size) noexcept {
            for (uint32_t i = 0; i <= get_capacity(); i++) {
                Index front{i};
                Index back{i + size - 1};
                if (test_range(front, back)) {
                    set_range(front, back);
                    return Index(i);
                }
            }

            return Index::eInvalid;
        }

        constexpr bool test_set(Index index) noexcept {
            if (test(index)) {
                return false;
            }

            set(index);
            return true;
        }
    };

    struct AtomicBitMap final : detail::BitSetStorage<std::atomic_uint64_t, AtomicBitMap> {
        using Super = detail::BitSetStorage<std::atomic_uint64_t, AtomicBitMap>;
        using Super::BitSetStorage;

        bool test_set(Index index) noexcept;

        // TODO: should be possible to set some ranges atomically
        // as long as the range doesnt cross word boundaries
    };
}
