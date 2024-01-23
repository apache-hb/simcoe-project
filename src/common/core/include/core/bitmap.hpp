#pragma once

#include "core/unique.hpp"

#include <atomic>
#include <bit>

#include "core.reflect.h"

namespace sm {
    namespace detail {
        template<typename T, typename Super>
        struct BitMapStorage {
            struct Index : public sm::BitMapIndex { using BitMapIndex::BitMapIndex; };

            constexpr BitMapStorage() = default;

            constexpr BitMapStorage(size_t bits)
                : m_size(bits)
                , m_bits(word_count())
            {
                reset();
            }

            constexpr void resize(size_t bits) {
                m_size = bits;
                m_bits = sm::UniquePtr<T[]>(word_count());
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
            constexpr size_t get_capacity() const { return word_count() * kBitPerWord; }

            constexpr Index scan_set_first() {
                Super *self = static_cast<Super*>(this);
                for (size_t i = 0; i < get_total_bits(); i++) {
                    if (self->test_set(i)) {
                        return Index(i);
                    }
                }

                return Index::eInvalid;
            }

            constexpr void reset() {
                if (m_bits.is_valid())
                    std::memset(m_bits.get(), 0, word_count() * sizeof(T));
            }

            constexpr static inline size_t kBitPerWord = sizeof(T) * CHAR_BIT;

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
            constexpr T get_mask(Index bit) const { return T(1) << (bit.as_integral() % kBitPerWord); }
            constexpr size_t get_word(Index bit) const { return bit.as_integral() / kBitPerWord; }
            constexpr size_t word_count() const { return (get_total_bits() / kBitPerWord) + 1; }

            size_t m_size;
            sm::UniquePtr<T[]> m_bits;

            constexpr void verify_index(Index index) const {
                CTASSERTF(index != Index::eInvalid, "invalid index");
                CTASSERTF(index.as_integral() <= get_total_bits(), "bit %zu is out of bounds", index.as_integral());
            }
        };
    }

    struct BitMap final : detail::BitMapStorage<std::uint64_t, BitMap> {
        using Super = detail::BitMapStorage<std::uint64_t, BitMap>;
        using Super::BitMapStorage;

        // no way to garuntee that this is done atomically
        // so put it in BitMap instead of AtomicBitMap
        constexpr void set_range(Index front, Index back) {
            verify_index(front);
            verify_index(back);

            for (auto i : Index::range(front, back)) {
                set(i.as_enum()); // TODO: optimize
            }
        }

        constexpr bool test_range(Index front, Index back) {
            verify_index(front);
            verify_index(back);

            for (auto i : Index::range(front, back)) {
                if (!test(i.as_enum())) {
                    return false;
                }
            }

            return true;
        }

        constexpr Index scan_set_range(Index size) {
            for (auto i : Index::range(Index::kBegin, Index(get_total_bits()))) {
                auto front = i.as_integral();
                auto back = i.as_integral() + size.as_integral() - 1;
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
    };
}
