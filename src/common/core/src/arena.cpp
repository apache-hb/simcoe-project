#include "core/arena.hpp"
#include "core/macros.h"

#include <string.h>
#include <stdlib.h>

using namespace sm;

// trampoline functions

void *IArena::wrap_alloc(size_t size, void *user) {
    return static_cast<IArena*>(user)->alloc(size);
}

void *IArena::wrap_resize(void *ptr, size_t new_size, size_t old_size, void *user) {
    return static_cast<IArena*>(user)->resize(ptr, new_size, old_size);
}

void IArena::wrap_release(void *ptr, size_t size, void *user) {
    static_cast<IArena*>(user)->release(ptr, size);
}

void IArena::wrap_rename(const void *ptr, const char *name, void *user) {
    static_cast<IArena*>(user)->rename(ptr, name);
}

void IArena::wrap_reparent(const void *ptr, const void *parent, void *user) {
    static_cast<IArena*>(user)->reparent(ptr, parent);
}

// public interface

void *IArena::alloc(size_t size) {
    return impl_alloc(size);
}

void *IArena::resize(void *ptr, size_t new_size, size_t old_size) {
    return impl_resize(ptr, new_size, old_size);
}

void IArena::release(void *ptr, size_t size) {
    impl_release(ptr, size);
}

void IArena::rename(const void *ptr, const char *ptr_name) {
    impl_rename(ptr, ptr_name);
}

void IArena::reparent(const void *ptr, const void *parent_ptr) {
    impl_reparent(ptr, parent_ptr);
}

// default implementations

void IArena::impl_release(void *ptr, size_t size) {
    CT_UNUSED(ptr);
    CT_UNUSED(size);
}

void IArena::impl_rename(const void *ptr, const char *ptr_name) {
    CT_UNUSED(ptr);
    CT_UNUSED(ptr_name);
}

void IArena::impl_reparent(const void *ptr, const void *parent_ptr) {
    CT_UNUSED(ptr);
    CT_UNUSED(parent_ptr);
}

class DefaultArena final : public IArena {
    using IArena::IArena;

    void *impl_alloc(size_t size) override {
        return ::malloc(size);
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        CT_UNUSED(old_size);

        return ::realloc(ptr, new_size);
    }

    void impl_release(void *ptr, size_t size) override {
        CT_UNUSED(size);

        ::free(ptr);
    }
};

constinit static DefaultArena gGlobalArena{"global"};

arena_t *sm::global_arena() {
    return &gGlobalArena;
}
