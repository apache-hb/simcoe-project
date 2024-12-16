#pragma once

#include "base/macros.hpp"

namespace sm {
    class SlabAllocator {
        using This = SlabAllocator;

        void *mMemory;
        size_t mCapacity;
        size_t mBlockSize;

    public:
        SlabAllocator(void *memory, size_t capacity, size_t blockSize) noexcept
            : mMemory(memory)
            , mCapacity(capacity)
            , mBlockSize(blockSize)
        { }

        SlabAllocator(size_t capacity, size_t blockSize);

        ~SlabAllocator() noexcept;

        SM_NOCOPY(SlabAllocator);
        SM_NOMOVE(SlabAllocator);

        void *allocate() noexcept;
        void deallocate(void *ptr) noexcept;

        size_t capacity() const noexcept { return mCapacity; }
        size_t blockSize() const noexcept { return mBlockSize; }
        size_t totalSize() const noexcept { return mCapacity * mBlockSize; }
    };
}
