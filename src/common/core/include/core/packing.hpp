#pragma once

#include "base/panic.h"
#include "ctu/reflect.h"

#include "io/io.h"

namespace sm {
    template<typename T>
    void serialize(const T& data, io_t *io);

    template<>
    inline void serialize<bool>(const bool& data, io_t *io) {
        io_write(io, &data, sizeof(data));
    }

    template<>
    inline void serialize<uint32_t>(const uint32_t& data, io_t *io) {
        io_write(io, &data, sizeof(data));
    }

    template<>
    inline void serialize<int32_t>(const int32_t& data, io_t *io) {
        io_write(io, &data, sizeof(data));
    }

    template<>
    inline void serialize<const char*>(const char *const& data, io_t *io) {
        size_t size = data ? std::strlen(data) : 0;
        CTASSERTF(size <= UINT32_MAX, "string too long: %zu", size);

        serialize(uint32_t(size), io);
        io_write(io, data, size);
    }

    template<typename T>
    inline void serialize(const T& data, io_t *io) {
        constexpr auto refl = ctu::reflect<T>();
        constexpr auto fields = refl.get_fields();

        for (size_t i = 0; i < fields.size(); i++) {
            refl.visit(data, i, [io](const auto& value) {
                serialize(value, io);
            });
        }
    }
}
