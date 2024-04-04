#pragma once

#include "core/unique.hpp"
#include "core/error.hpp"
#include "core/win32.hpp" // IWYU pragma: export
#include "os/os.h"

#include "io.reflect.h"

namespace sm {
    constexpr auto kIoClose = [](io_t *io) {
        CTASSERTF(io_close(io) == eOsSuccess, "Failed to close io handle");
    };

    using IoHandle = sm::UniquePtr<io_t, decltype(kIoClose)>;

    struct Io : IoHandle {
        static Io file(const char *path, archive::IoAccess access);

        OsError error() const;
        const char *name() const;

        size_t size() const;
        size_t read_bytes(void *data, size_t size);
        size_t write_bytes(const void *data, size_t size);
    };
}
