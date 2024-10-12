#include "stdafx.hpp"

#include "core/allocators/slab.hpp"

using SlabAllocator = sm::SlabAllocator;

static void *slabMalloc(size_t size) {
    void *ptr = std::malloc(size);
    if (ptr == nullptr)
        throw std::bad_alloc();

    return ptr;
}

SlabAllocator::SlabAllocator(size_t capacity, size_t blockSize)
    : This(slabMalloc(capacity * blockSize), capacity, blockSize)
{ }

SlabAllocator::~SlabAllocator() noexcept {
    std::free(mMemory);
}


void *SlabAllocator::allocate() noexcept {
    return nullptr;
}

void SlabAllocator::deallocate(void *ptr) noexcept {

}
