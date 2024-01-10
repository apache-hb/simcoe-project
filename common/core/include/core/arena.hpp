#pragma once

#include <core_api.hpp>

namespace simcoe {
    enum class AllocError {
        NotImplemented,
        OutOfMemory,
        Success
    };

    class SM_CORE_API IArena {
    protected:
        virtual AllocError impl_malloc(size_t size, void **ptr) = 0;
        virtual AllocError impl_realloc(void *ptr, size_t new_size, size_t old_size, void **new_ptr) = 0;
        virtual AllocError impl_free(void *ptr, size_t size) = 0;

    public:
        virtual ~IArena() = default;

        void *malloc(size_t size);
        void *realloc(void *ptr, size_t new_size);
        void free(void *ptr);
    };
}
