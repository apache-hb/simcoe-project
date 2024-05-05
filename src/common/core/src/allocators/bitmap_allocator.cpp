#include "stdafx.hpp"

#include "core/allocators/bitmap_allocator.hpp"

using namespace sm;

size_t BitMapIndexAllocator::acquireFirstFreeIndex() noexcept {
    for (size_t i = 0; i < mBitSet.getBitCapacity(); ++i) {
        if (!mBitSet.test(i)) {
            mBitSet.set(i);
            return i;
        }
    }

    return kInvalidIndex;
}

size_t BitMapIndexAllocator::acquireFirstFreeRange(size_t count) noexcept {
    // search for a range of free bits of at least count size

    size_t start = 0;
    size_t end = 0;

    for (size_t i = 0; i < mBitSet.getBitCapacity(); ++i) {
        if (!mBitSet.test(i)) {
            if (start == 0) {
                start = i;
            }

            end = i;

            if (end - start + 1 >= count) {
                for (size_t j = start; j <= end; ++j) {
                    mBitSet.set(j);
                }

                return start;
            }
        }
    }

    return kInvalidIndex;
}

size_t BitMapIndexAllocator::allocateIndex() noexcept {
    return acquireFirstFreeIndex();
}

size_t BitMapIndexAllocator::allocateRange(size_t count) noexcept {
    return acquireFirstFreeRange(count);
}

void BitMapIndexAllocator::deallocate(size_t index) noexcept {
    mBitSet.clear(index);
}

void BitMapIndexAllocator::deallocateRange(size_t index, size_t count) noexcept {
    for (size_t i = index; i < index + count; ++i) {
        mBitSet.clear(i);
    }
}

void BitMapIndexAllocator::setRange(size_t first, size_t last) noexcept {
    mBitSet.setRange(first, last);
}
