#include "core/arena.hpp"
#include "core/macros.h"
#include "core/macros.hpp"

#include <cstring>
#include <cstdlib>

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

constinit static DefaultArena gGlobalPools[] = {
    /* global */ DefaultArena{"global"},
    /* debug */ DefaultArena{"debug"},
    /* logging */ DefaultArena{"logging"},
    /* rhi */ DefaultArena{"rhi"},
    /* render */ DefaultArena{"render"},
    /* assets */ DefaultArena{"assets"},
};

IArena& sm::get_pool(Pool pool) {
    SM_UNUSED constexpr auto refl = ctu::reflect<Pool>();
    CTASSERTF(pool.is_valid(), "invalid pool %s", refl.to_string(pool).data());

    return gGlobalPools[pool.as_integral()];
}
