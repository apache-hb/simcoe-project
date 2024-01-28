#pragma once

#include "core/traits.hpp" // IWYU pragma: export
#include "core/unique.hpp"

#include <bit>

namespace sm {
    namespace detail {
        /// @brief bitmap backing storage
        /// @tparam T the type of the backing storage
        /// @tparam Super crtp superclass
        template<Integral T, typename Super>
        struct BitMapStorage {
            enum Index : size_t { eInvalid = SIZE_MAX };
            using IndexType = std::underlying_type_t<Index>;

            constexpr BitMapStorage() = default;

            constexpr BitMapStorage(size_t length)
                : m_size(length)
            {
                resize(length);
            }

            constexpr void resize(size_t length) {
                if (length > m_size) {
                    m_size = length;
                    m_bits.reset(word_count());
                }

                reset();
            }

            constexpr size_t popcount() const {
                size_t count = 0;
                for (size_t i = 0; i < word_count(); i++) {
                    count += std::popcount(m_bits[i]);
                }
                return count;
            }

            constexpr size_t freecount() const {
                return get_capacity() - popcount();
            }

            constexpr size_t get_total_bits() const { return m_size; }
            constexpr size_t get_capacity() const { return word_count() * kBitsPerWord; }

            constexpr Index scan_set_first() {
                Super *self = static_cast<Super*>(this);
                for (uint32_t i = 0; i < get_total_bits(); i++) {
                    if (self->test_set(Index{i})) {
                        return Index{i};
                    }
                }

                return Index::eInvalid;
            }

            constexpr bool is_valid() const { return m_bits.is_valid(); }

            constexpr void reset() {
                CTASSERT(is_valid());
                std::memset(m_bits.get(), 0, word_count());
            }

            constexpr void release(Index index) {
                CTASSERT(test(index));
                clear(index);
            }

            constexpr static inline size_t kBitsPerWord = sizeof(T) * CHAR_BIT;

            constexpr bool test(Index index) const {
                verify_index(index);

                return m_bits[get_word(index)] & get_mask(index);
            }

            constexpr void set(Index index) {
                verify_index(index);
                m_bits[get_word(index)] |= get_mask(index);
            }

            constexpr void clear(Index index) {
                verify_index(index);
                m_bits[get_word(index)] &= ~get_mask(index);
            }

        protected:
            constexpr T get_mask(Index bit) const { return T(1) << (bit % kBitsPerWord); }
            constexpr size_t get_word(Index bit) const { return bit / kBitsPerWord; }
            constexpr size_t word_count() const { return (get_total_bits() / kBitsPerWord) + 1; }

            size_t m_size = 0;
            sm::UniquePtr<T[]> m_bits;

            constexpr void verify_index(Index index) const {
                CTASSERTF(index != Index::eInvalid, "invalid index");
                CTASSERTF(index <= get_total_bits(), "bit %zu is out of bounds", index);
            }
        };
    }

    // TODO: these shouldnt handle allocation, thats a seperate concern
    // these should just be the underlying storage

    struct BitMap final : detail::BitMapStorage<std::uint64_t, BitMap> {
        using Super = detail::BitMapStorage<std::uint64_t, BitMap>;
        using Super::BitMapStorage;

        // no way to garuntee that this is done atomically
        // so put it in BitMap instead of AtomicBitMap
        constexpr void set_range(Index front, Index back) {
            verify_index(front);
            verify_index(back);

            for (size_t i = front; i <= back; i++) {
                set(Index(i)); // TODO: optimize
            }
        }

        constexpr bool test_range(Index front, Index back) {
            verify_index(front);
            verify_index(back);

            for (uint32_t i = front; i <= back; i++) {
                if (!test(Index{i})) {
                    return false;
                }
            }

            return true;
        }

        constexpr Index scan_set_range(Index size) {
            for (uint32_t i = 0; i <= get_total_bits(); i++) {
                Index front{i};
                Index back{i + size - 1};
                if (test_range(front, back)) {
                    set_range(front, back);
                    return Index(i);
                }
            }

            return Index::eInvalid;
        }

        constexpr bool test_set(Index index) {
            if (test(index)) {
                return false;
            }

            set(index);
            return true;
        }
    };

    struct AtomicBitMap final : detail::BitMapStorage<std::atomic_uint64_t, AtomicBitMap> {
        using Super = detail::BitMapStorage<std::atomic_uint64_t, AtomicBitMap>;
        using Super::BitMapStorage;

        bool test_set(Index index);

        // TODO: should be possible to set some ranges atomically
        // as long as the range doesnt cross word boundaries
    };
}
