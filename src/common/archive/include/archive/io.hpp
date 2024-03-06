#pragma once

#include "core/unique.hpp"
#include "core/error.hpp"

#include "io.reflect.h"

namespace sm {
    using IoHandle = sm::FnUniquePtr<io_t, io_close>;

    struct Io : IoHandle {
        static Io file(const char *path, archive::IoAccess access);

        OsError error() const;
        const char *name() const;

        size_t size() const;
        size_t read_bytes(void *data, size_t size);
        size_t write_bytes(const void *data, size_t size);
    };
}
