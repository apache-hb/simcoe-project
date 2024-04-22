#pragma once

#include "os/os.h"

#include "core/error.hpp"
#include "core/unique.hpp"
#include "core/fs.hpp"

constexpr bool operator!=(const os_library_t& lhs, const os_library_t& rhs) {
    return lhs.impl != rhs.impl;
}

namespace sm {
    using LibraryHandle = sm::UniqueHandle<os_library_t, decltype([](os_library_t& it) { os_library_close(&it); })>;

    class Library : LibraryHandle {
        OsError mError = 0;

        void *get_symbol(const char *name);

    public:
        Library() = default;
        Library(const char *path);
        Library(const fs::path& path);

        OsError get_error() const;

        template<typename F>
        F fn(const char *name) {
            return reinterpret_cast<F>(get_symbol(name));
        }

        template<typename T>
        T *var(const char *name) {
            return reinterpret_cast<T*>(get_symbol(name));
        }
    };
}
