#pragma once

#include "core/core.hpp"
#include "core/string.hpp"

#include "base/panic.h"

namespace sm {
    class SmallStringBase {
    protected:
        int16 mLength; // length not including null terminator

        void init(char *buffer, const char *str, int16 length);
        void init(char *buffer, const char *str);
    };

    template<size_t N> requires (N > 0)
    class SmallString : private SmallStringBase {
        // round up to next multiple of 2 for better alignment
        static constexpr size_t kAlignedSize = (N + 1 + 1) & ~1;

        char mBuffer[kAlignedSize];

    public:
        SmallString() = default;

        SmallString(const char *str) {
            SmallStringBase::init(mBuffer, str);
        }

        SmallString(const char *str, int16 length) {
            SmallStringBase::init(mBuffer, str, length);
        }

        template<size_t M>
        SmallString(const SmallString<M> &other) {
            static_assert(N >= M);

            SmallStringBase::init(mBuffer, other.data(), other.size());
        }

        template<size_t M>
        SmallString(const char (&str)[M]) {
            static_assert(N >= M);

            SmallStringBase::init(mBuffer, str, M - 1);
        }

        SmallString(sm::StringView str) {
            SmallStringBase::init(mBuffer, str.data(), str.size());
        }

        SmallString(const sm::String &str) {
            CTASSERTF(str.size() <= N, "String too long (%zu > %zu)", str.size(), N);
            SmallStringBase::init(mBuffer, str.data(), str.size());
        }

        const char *data() const { return mBuffer; }
        int16 size() const { return mLength; }
        const char *c_str() const { return mBuffer; }
    };
}