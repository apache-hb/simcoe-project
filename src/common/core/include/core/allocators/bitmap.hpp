#pragma once

#include "core/adt/bitset.hpp"

namespace sm {
    class BitMapIndexAllocator;

    class BitMapIndexIterator {
        const BitMapIndexAllocator& mAllocator;
        size_t mIndex;

    public:
        BitMapIndexIterator(const BitMapIndexAllocator& allocator, size_t index) noexcept
            : mAllocator(allocator)
            , mIndex(index)
        { }

        size_t operator*() const noexcept { return mIndex; }

        BitMapIndexIterator& operator++() noexcept;

        bool operator==(const BitMapIndexIterator& other) const noexcept;
    };

    class BitMapIndexAllocator {
        friend BitMapIndexIterator;

        adt::BitSet mBitSet;

        size_t acquireFirstFreeIndex() noexcept;
        size_t acquireFirstFreeRange(size_t count) noexcept;

    public:
        static constexpr inline size_t kInvalidIndex = SIZE_MAX;

        BitMapIndexAllocator(size_t capacity) noexcept
            : mBitSet(capacity)
        { }

        size_t capacity() const noexcept { return mBitSet.getBitCapacity(); }
        void resize(size_t capacity) noexcept { mBitSet.resize(capacity); }
        void resizeAndClear(size_t capacity) noexcept { mBitSet.resizeAndClear(capacity); }
        void clear() noexcept { mBitSet.clear(); }

        bool test(size_t index) const noexcept { return mBitSet.test(index); }

        size_t popcount() const noexcept { return mBitSet.popcount(); }
        size_t freecount() const noexcept { return mBitSet.freecount(); }

        size_t allocateIndex() noexcept;
        size_t allocateRange(size_t count) noexcept;

        void deallocate(size_t index) noexcept;
        void deallocateRange(size_t index, size_t count) noexcept;

        void setRange(size_t first, size_t last) noexcept;

        BitMapIndexIterator begin() const noexcept { return { *this, 0 }; }
        BitMapIndexIterator end() const noexcept { return { *this, mBitSet.getBitCapacity() }; }
    };

    template<typename T>
    class BitMapAllocator : public BitMapIndexAllocator {
        using Super = BitMapIndexAllocator;

        sm::UniquePtr<T[]> mStorage;

    public:
        BitMapAllocator(size_t capacity) noexcept
            : BitMapIndexAllocator(capacity)
            , mStorage(new T[capacity])
        { }

        constexpr void resize(size_t capacity) noexcept {
            Super::resize(capacity);
            mStorage.reset(new T[capacity]);
        }

        constexpr T *allocateElement() noexcept {
            size_t index = allocateIndex();
            return index != kInvalidIndex ? &mStorage[index] : nullptr;
        }

        constexpr T *allocateElementRange(size_t count) noexcept {
            size_t index = allocateRange(count);
            return index != kInvalidIndex ? &mStorage[index] : nullptr;
        }

        constexpr auto *getElement(this auto& self, size_t index) noexcept {
            return self.test(index) ? &self.mStorage[index] : nullptr;
        }
    };
}
