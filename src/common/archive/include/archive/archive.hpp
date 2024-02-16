#pragma once

#include "core/macros.hpp"
#include "core/core.hpp"
#include "core/span.hpp"
#include "core/vector.hpp"
#include "core/unique.hpp"
#include "core/text.hpp"

#include "io/io.h"

#include <stdint.h>

namespace sm {
    class Archive {
        using IoHandle = sm::FnUniquePtr<io_t, io_close>;
        IoHandle mStream;

    public:
        SM_NOCOPY(Archive)

        constexpr Archive(io_t *stream)
            : mStream(stream)
        { }

        constexpr bool is_valid() const { return mStream.is_valid(); }

        void write_bytes(const void *data, size_t size);
        size_t read_bytes(void *data, size_t size);

        template<StandardLayout T>
        void write(const T& data) {
            write_bytes(reinterpret_cast<const void*>(&data), sizeof(T));
        }

        template<StandardLayout T>
        void write_many(sm::Span<const T> data) {
            uint32_t count = static_cast<uint32_t>(data.size());
            write(count);
            write_bytes(data.data(), data.size_bytes());
        }

        template<StandardLayout T>
        bool read(T& data) {
            return read_bytes(reinterpret_cast<void*>(&data), sizeof(T)) == sizeof(T);
        }

        template<StandardLayout T>
        bool read_many(sm::Vector<T>& data) {
            uint32_t count;
            if (!read(count))
                return false;

            data.resize(count);
            return read_bytes(data.data(), data.size_bytes()) == data.size_bytes();
        }

        void write_string(sm::StringView str);
        bool read_string(sm::String& str);

        static Archive load(sm::StringView path);
        static Archive save(sm::StringView path);
    };
}
