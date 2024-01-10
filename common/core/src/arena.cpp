#include "core/arena.hpp"

using namespace simcoe;

void *IArena::malloc(size_t size) {
    void *ptr = nullptr;
    AllocError error = impl_malloc(size, &ptr);
}
