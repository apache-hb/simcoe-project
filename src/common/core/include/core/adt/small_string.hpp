#pragma once

#include "core/core.hpp"
#include "core/string.hpp"

#include "base/panic.h"

#include <cstring>
#include <memory>

namespace sm {
    template<size_t N> requires (N > 0)
    class SmallString {
        // round up to next multiple of 2 for better alignment
        static constexpr size_t kAlignedSize = (N + 1 + 1) & ~1;

        char mBuffer[kAlignedSize];
        uint16_t mLength;

    public:
        static constexpr size_t kCapacity = N;

        constexpr SmallString() noexcept = default;

        constexpr SmallString(const char *first, const char *last) noexcept {
            mLength = last - first;
            CTASSERTF(mLength <= N, "String too long (%hu > %zu)", mLength, N);

            std::uninitialized_copy(first, last, mBuffer);
            mBuffer[mLength] = '\0';
        }

        constexpr SmallString(const char *str) noexcept
            : SmallString(str, str + std::strlen(str))
        { }

        constexpr SmallString(const char *str, int16_t length) noexcept
            : SmallString(str, str + length)
        { }

        template<size_t M> requires (M <= N)
        constexpr SmallString(const SmallString<M> &other) noexcept
            : SmallString(other.data(), other.size())
        { }

        template<size_t M>
        constexpr SmallString(const char (&str)[M]) noexcept
            : SmallString(str, M - 1)
        { }

        constexpr SmallString(sm::StringView str) noexcept
            : SmallString(str.data(), str.size())
        { }

        constexpr SmallString(const sm::String &str) noexcept
            : SmallString(str.data(), str.size())
        { }

        constexpr explicit operator std::string() const noexcept {
            return std::string(mBuffer, mLength);
        }

        [[nodiscard]] constexpr const char *data() const noexcept { return mBuffer; }
        [[nodiscard]] constexpr int16_t size() const noexcept { return mLength; }
        [[nodiscard]] constexpr const char *c_str() const noexcept { return mBuffer; }
    };
}