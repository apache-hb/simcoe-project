#pragma once

#include "arena/arena.h"

namespace sm {
class IArena : public arena_t {
    static void *wrap_alloc(size_t size, void *user);
    static void *wrap_resize(void *ptr, size_t new_size, size_t old_size, void *user);
    static void wrap_release(void *ptr, size_t size, void *user);
    static void wrap_rename(const void *ptr, const char *name, void *user);
    static void wrap_reparent(const void *ptr, const void *parent, void *user);

protected:
    virtual void *impl_alloc(size_t size) = 0;
    virtual void *impl_resize(void *ptr, size_t new_size, size_t old_size) = 0;
    virtual void impl_release(void *ptr, size_t size);

    // memory tracing functions
    virtual void impl_rename(const void *ptr, const char *ptr_name);
    virtual void impl_reparent(const void *ptr, const void *parent);

public:
    constexpr IArena(const char *id)
        : arena_t({
              .name = id,
              .fn_malloc = wrap_alloc,
              .fn_realloc = wrap_resize,
              .fn_free = wrap_release,
              .fn_rename = wrap_rename,
              .fn_reparent = wrap_reparent,
              .user = this,
          }) {}

    virtual ~IArena() = default;

    void *alloc(size_t size);
    void *resize(void *ptr, size_t new_size, size_t old_size);
    void release(void *ptr, size_t size);

    void rename(const void *ptr, const char *ptr_name);
    void reparent(const void *ptr, const void *parent);
};

arena_t *global_arena();
} // namespace sm
