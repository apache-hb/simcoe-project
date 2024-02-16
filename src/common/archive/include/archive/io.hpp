#pragma once

#include "core/unique.hpp"

#include "core/arena.hpp"

#include "fmtlib/format.h" // IWYU pragma: keep

#include "io.reflect.h"

namespace sm {
    using IoHandle = sm::FnUniquePtr<io_t, io_close>;

    class IoError {
        io_error_t mError;

    public:
        constexpr IoError(io_error_t error)
            : mError(error)
        { }

        constexpr io_error_t error() const { return mError; }
        constexpr operator bool() const { return mError != 0; }
        constexpr bool success() const { return mError == 0; }
        constexpr bool failed() const { return mError != 0; }
    };

    struct Io : IoHandle {
        static Io file(const char *path, archive::IoAccess access);

        IoError error() const;
        const char *name() const;

        size_t size() const;
        size_t read_bytes(void *data, size_t size);
        size_t write_bytes(const void *data, size_t size);
    };
}

template<>
struct fmt::formatter<sm::IoError> {
    constexpr auto parse(format_parse_context &ctx) const {
        return ctx.begin();
    }

    auto format(const sm::IoError &error, fmt::format_context &ctx) const {
        auto& pool = sm::get_pool(sm::Pool::eDebug);
        return format_to(ctx.out(), "{}", os_error_string(error.error(), &pool));
    }
};
