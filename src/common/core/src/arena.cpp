#include "core/arena.hpp"
#include "core/macros.h"

#include <cstring>
#include <cstdlib>

using namespace sm;

// trampoline functions

static void *wrap_alloc(size_t size, void *user) {
    return static_cast<IArena*>(user)->alloc(size);
}

static void *wrap_resize(void *ptr, size_t new_size, size_t old_size, void *user) {
    return static_cast<IArena*>(user)->resize(ptr, new_size, old_size);
}

static void wrap_release(void *ptr, size_t size, void *user) {
    static_cast<IArena*>(user)->release(ptr, size);
}

static void wrap_rename(const void *ptr, const char *name, void *user) {
    static_cast<IArena*>(user)->rename(ptr, name);
}

static void wrap_reparent(const void *ptr, const void *parent, void *user) {
    static_cast<IArena*>(user)->reparent(ptr, parent);
}

// setup

void IArena::init(const char *id) {
    name = id;
    fn_malloc = wrap_alloc;
    fn_realloc = wrap_resize;
    fn_free = wrap_release;
    fn_rename = wrap_rename;
    fn_reparent = wrap_reparent;
    user = this;
}

// public interface

IArena::IArena(const char *name) {
    init(name);
}

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

void IArena::reparent(const void *ptr, const void *parent) {
    impl_reparent(ptr, parent);
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

void IArena::impl_reparent(const void *ptr, const void *parent) {
    CT_UNUSED(ptr);
    CT_UNUSED(parent);
}

class DefaultArena final : public IArena {
    using IArena::IArena;

    void *impl_alloc(size_t size) override {
        return std::malloc(size);
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        CT_UNUSED(old_size);

        return std::realloc(ptr, new_size);
    }

    void impl_release(void *ptr, size_t size) override {
        CT_UNUSED(size);

        std::free(ptr);
    }
};

static DefaultArena gDebugArena{"debug"};

IArena *sm::get_debug_arena(void) {
    return &gDebugArena;
}
