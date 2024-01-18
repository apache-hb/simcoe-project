#pragma once

#include <sm_core_api.hpp>

#include "arena/arena.h"

namespace sm {
    class SM_CORE_API IArena : public arena_t {
        void init(const char *id);

    protected:
        virtual void *impl_alloc(size_t size) = 0;
        virtual void *impl_resize(void *ptr, size_t new_size, size_t old_size) = 0;
        virtual void impl_release(void *ptr, size_t size);

        // memory tracing functions
        virtual void impl_rename(const void *ptr, const char *ptr_name);
        virtual void impl_reparent(const void *ptr, const void *parent);

    public:
        IArena(const char *name);

        virtual ~IArena() = default;

        void *alloc(size_t size);
        void *resize(void *ptr, size_t new_size, size_t old_size);
        void release(void *ptr, size_t size);

        void rename(const void *ptr, const char *ptr_name);
        void reparent(const void *ptr, const void *parent);
    };
}
