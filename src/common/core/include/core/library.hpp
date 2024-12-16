#pragma once

#ifdef _WIN32
#include "core/error.hpp"
#include "core/memory/unique.hpp"
#include "base/fs.hpp"

#include "os/os.h"

#include <expected>

constexpr bool operator!=(const os_library_t& lhs, const os_library_t& rhs) {
    return lhs.impl != rhs.impl;
}

namespace sm {
    using LibraryHandle = sm::UniqueHandle<
        os_library_t,
        decltype([](os_library_t& it) { os_library_close(&it); })
    >;

    class Library {
        LibraryHandle mHandle;
        OsError mError = 0;

        void *getSymbol(const char *name) noexcept;

        Library(os_library_t library) noexcept
            : mHandle(library)
        { }

    public:
        Library() noexcept = default;
        Library(const char *path);
        Library(const fs::path& path);

        static std::expected<Library, OsError> open(const fs::path& path) noexcept;

        OsError getError() const noexcept;

        template<typename F>
        F fn(const char *name) {
            return reinterpret_cast<F>(getSymbol(name));
        }

        template<typename T>
        T *var(const char *name) {
            return reinterpret_cast<T*>(getSymbol(name));
        }
    };
}
#endif
