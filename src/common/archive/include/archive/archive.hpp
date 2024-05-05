#pragma once

#include <simcoe_config.h>

#include "core/adt/vector.hpp"
#include "core/string.hpp"
#include "core/traits.hpp"

#include "archive/io.hpp"

#include <stdint.h>

namespace sm {
    class Archive {
        IoHandle mStream;

    public:
        constexpr Archive(io_t *stream)
            : mStream(stream)
        { }

        constexpr bool is_valid() const { return mStream.is_valid(); }

        void write_bytes(const void *data, size_t size);
        void write_string(sm::StringView str);
        void write_length(size_t length);

        size_t read_bytes(void *data, size_t size);
        bool read_string(sm::String& str);
        bool read_length(size_t& length);

        template<StandardLayout T>
        void write(const T& data) {
            write_bytes(reinterpret_cast<const void*>(&data), sizeof(T));
        }

        template<StandardLayout T>
        void write_many(const sm::Vector<T>& data) {
            write_length(data.size());
            write_bytes(data.data(), data.size() * sizeof(T));
        }

        template<StandardLayout T>
        bool read(T& data) {
            return read_bytes(reinterpret_cast<void*>(&data), sizeof(T)) == sizeof(T);
        }

        template<StandardLayout T>
        bool read_many(sm::Vector<T>& data) {
            size_t count;
            if (!read_length(count))
                return false;

            size_t size = count * sizeof(T);

            data.resize(count);
            return read_bytes(data.data(), size) == size;
        }

        static Archive load(sm::StringView path);
        static Archive save(sm::StringView path);
    };
}
