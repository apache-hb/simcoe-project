#pragma once

#include "arena/arena.h"
#include "base/panic.h"

#include "core.reflect.h"

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
        constexpr IArena(const char *id) {
            name = id;
            fn_malloc = wrap_alloc;
            fn_realloc = wrap_resize;
            fn_free = wrap_release;
            fn_rename = wrap_rename;
            fn_reparent = wrap_reparent;
            user = this;
        }

        virtual ~IArena() = default;

        void *alloc(size_t size);
        void *resize(void *ptr, size_t new_size, size_t old_size);
        void release(void *ptr, size_t size);

        void rename(const void *ptr, const char *ptr_name);
        void reparent(const void *ptr, const void *parent);
    };

    IArena& get_pool(Pool pool);

    template<typename T>
    class StandardArena {
        IArena& m_arena;

    public:
        constexpr IArena& get_arena() const noexcept { return m_arena; }
        using value_type = T;

        constexpr bool operator==(const StandardArena &) const noexcept { return true; }
        constexpr bool operator!=(const StandardArena &) const noexcept { return false; }

        T *allocate(size_t n) const {
            if (n > SIZE_MAX / sizeof(T)) {
                return nullptr;
            }

            if (auto p = static_cast<T *>(m_arena.alloc(n * sizeof(T)))) {
                return p;
            }

            NEVER("Arena %s failed to allocate %zu bytes", m_arena.name, n * sizeof(T));
        }

        void deallocate(T *const p, size_t n) const noexcept {
            m_arena.release(p, n * sizeof(T));
        }

        constexpr StandardArena(const StandardArena &) noexcept = default;
        constexpr StandardArena(StandardArena &&) noexcept = default;

        template<typename O>
        constexpr StandardArena(const StandardArena<O> &o) noexcept
            : m_arena(o.get_arena())
        { }

        StandardArena(Pool poll = Pool::eGlobal) noexcept
            : m_arena(get_pool(poll))
        { }

        constexpr StandardArena(IArena& arena) noexcept
            : m_arena(arena)
        { }
    };

    template<typename T, Pool::Inner P> requires (Pool{P}.is_valid())
    struct StandardPool {
        using value_type = T;

        constexpr IArena& get_arena() const noexcept { return sm::get_pool(P); }

        constexpr bool operator==(const StandardPool &) const noexcept { return true; }
        constexpr bool operator!=(const StandardPool &) const noexcept { return false; }

        T *allocate(size_t n) const {
            if (n > SIZE_MAX / sizeof(T)) {
                return nullptr;
            }

            if (auto p = static_cast<T *>(get_arena().alloc(n * sizeof(T)))) {
                return p;
            }

            NEVER("Arena %s failed to allocate %zu bytes", get_arena().name, n * sizeof(T));
        }

        void deallocate(T *const p, size_t n) const noexcept {
            get_arena().release(p, n * sizeof(T));
        }

        constexpr StandardPool() noexcept = default;
        constexpr StandardPool(const StandardPool &) noexcept = default;

        template<typename O>
        constexpr StandardPool(const StandardPool<O, P> &) noexcept { }
    };
}
