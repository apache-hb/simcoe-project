#pragma once

#include "core/core.hpp"

namespace sm {
    class SmallStringBase {
    protected:
        int16 mLength; // length not including null terminator

        void init(char *buffer, const char *str, int16 length);
        void init(char *buffer, const char *str);
    };

    template<size_t N>
    class SmallString : private SmallStringBase {
        static_assert(N > 0);

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

        const char *data() const { return mBuffer; }
        int16 size() const { return mLength; }
        const char *c_str() const { return mBuffer; }
    };
}