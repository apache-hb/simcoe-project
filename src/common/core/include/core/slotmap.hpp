#pragma once

#include "core/array.hpp"
#include "core/error.hpp"

#include <atomic>
#include <stdint.h>

namespace sm {
    namespace detail {
        template<typename T, T TEmpty, typename Super>
        struct SlotStorage {
            enum Index : size_t { eInvalid = SIZE_MAX };
            using IndexType = __underlying_type(Index);

            using Storage = UniqueArray<T>;
            using Iterator = typename Storage::Iterator;
            using ConstIterator = typename Storage::ConstIterator;

            constexpr SlotStorage(size_t count)
                : mStorage(count, TEmpty)
            { }

            constexpr size_t length() const { return mStorage.length(); }

            constexpr const T& operator[](size_t index) const { verify_index(index); return mStorage[index]; }
            constexpr T& operator[](size_t index) { verify_index(index); return mStorage[index]; }

            constexpr const T& get(size_t index) const { verify_index(index); return mStorage[index]; }
            constexpr T& get(size_t index) { verify_index(index); return mStorage[index]; }

            constexpr Iterator begin() { return mStorage.begin(); }
            constexpr ConstIterator begin() const { return mStorage.begin(); }

            constexpr Iterator end() { return mStorage.end(); }
            constexpr ConstIterator end() const { return mStorage.end(); }

            constexpr bool test(size_t index, T value) const {
                verify_index(index);
                return mStorage[index] == value;
            }

            constexpr bool test(size_t index) const {
                return test(index, TEmpty);
            }

            constexpr size_t popcount() const {
                size_t count = 0;
                for (auto& it : mStorage) {
                    if (it != TEmpty) {
                        count++;
                    }
                }
                return count;
            }

            constexpr size_t freecount() const {
                return length() - popcount();
            }

            constexpr Index alloc(size_t limit, T value) {
                Super *super = static_cast<Super*>(this);
                for (size_t i = 0; i < limit; i++) {
                    if (super->cmpxchg(i, TEmpty, value) == TEmpty) {
                        return Index(i);
                    }
                }

                return Index::eInvalid;
            }

            constexpr Index alloc(T value) {
                return alloc(length(), value);
            }

            constexpr void release(size_t index, T expected) {
                Super *super = static_cast<Super*>(this);
                SM_UNUSED auto value = super->cmpxchg(index, expected, TEmpty);
                SM_ASSERTF(value == expected, "invalid release at {} value: {}, expected: {}", index, value, expected);
            }

            constexpr void release(size_t index) {
                set(index, TEmpty);
            }

        protected:
            constexpr void set(size_t index, T value) {
                verify_index(index);
                mStorage[index] = value;
            }

            Storage mStorage;

            constexpr void verify_index(size_t index) const {
                CTASSERTF(index < length(), "index out of bounds: (%zu < %zu)", index, length());
            }
        };
    }

    template<typename T, T TEmpty = T{}>
    struct SlotMap : public detail::SlotStorage<T, TEmpty, SlotMap<T, TEmpty>> {
        using Super = detail::SlotStorage<T, TEmpty, SlotMap<T, TEmpty>>;
        using Index = typename Super::Index;
        using IndexType = typename Super::IndexType;

        constexpr T cmpxchg(size_t index, T expected, T desired) {
            if (Super::test(index, expected)) {
                Super::set(index, desired);
                return expected;
            }

            return Super::get(index);
        }
    };

    template<typename T, T TEmpty = T{}> requires (std::atomic<T>::is_lock_free())
    struct AtomicSlotMap : public detail::SlotStorage<std::atomic<T>, TEmpty, AtomicSlotMap<std::atomic<T>, TEmpty>> {
        using Super = detail::SlotStorage<std::atomic<T>, TEmpty, AtomicSlotMap<std::atomic<T>, TEmpty>>;
        using Index = typename Super::Index;
        using IndexType = typename Super::IndexType;

        constexpr T cmpxchg(size_t index, T expected, T desired) {
            T value = expected;
            Super::get(index).compare_exchange_strong(value, desired);
            return value;
        }
    };
}
