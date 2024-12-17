#pragma once

#include "base/panic.h"

#include <stdint.h>
#include <assert.h>

#include <algorithm>

namespace sm {
    namespace detail {
        template<size_t N, typename T>
        struct SizedIntegerTraits { using type = void; };

        template<> struct SizedIntegerTraits<1, signed> { using type = int8_t; };
        template<> struct SizedIntegerTraits<2, signed> { using type = int16_t; };

        template<> struct SizedIntegerTraits<3, signed> { using type = int32_t; };
        template<> struct SizedIntegerTraits<4, signed> { using type = int32_t; };

        template<> struct SizedIntegerTraits<5, signed> { using type = int64_t; };
        template<> struct SizedIntegerTraits<6, signed> { using type = int64_t; };
        template<> struct SizedIntegerTraits<7, signed> { using type = int64_t; };
        template<> struct SizedIntegerTraits<8, signed> { using type = int64_t; };

        template<> struct SizedIntegerTraits<1, unsigned> { using type = uint8_t; };
        template<> struct SizedIntegerTraits<2, unsigned> { using type = uint16_t; };

        template<> struct SizedIntegerTraits<3, unsigned> { using type = uint32_t; };
        template<> struct SizedIntegerTraits<4, unsigned> { using type = uint32_t; };

        template<> struct SizedIntegerTraits<5, unsigned> { using type = uint64_t; };
        template<> struct SizedIntegerTraits<6, unsigned> { using type = uint64_t; };
        template<> struct SizedIntegerTraits<7, unsigned> { using type = uint64_t; };
        template<> struct SizedIntegerTraits<8, unsigned> { using type = uint64_t; };
    }

    template<size_t N, typename S = signed> requires (N > 0 && N <= 8)
    struct [[gnu::packed]] SizedInteger {
        uint8_t octets[N];

        using Integral = typename detail::SizedIntegerTraits<N, S>::type;
        static constexpr bool kIsSigned = std::is_signed_v<S>;
        static constexpr Integral kMax = kIsSigned ? (1LL << (N * 8 - 1)) - 1 : (1ull << (N * 8)) - 1;
        static constexpr Integral kMin = kIsSigned ? -(1LL << (N * 8 - 1)) : 0;

        constexpr SizedInteger() noexcept = default;

        constexpr SizedInteger(Integral value) noexcept {
            store(value);
        }

        constexpr operator Integral() const noexcept {
            return load();
        }

        constexpr Integral load() const noexcept {
            Integral value;
            memcpy(&value, octets, N);
            return value;
        }

        constexpr void store(Integral value) noexcept {
            CTASSERT(value >= kMin && value <= kMax);
            memcpy(octets, &value, N);
        }

        constexpr SizedInteger& operator=(Integral value) noexcept {
            store(value);
            return *this;
        }

        constexpr SizedInteger byteswap() const noexcept {
            SizedInteger result;
            std::reverse_copy(octets, octets + N, result.octets);
            return result;
        }
    };

    using uint24_t = SizedInteger<3, unsigned>;
    using int24_t = SizedInteger<3, signed>;

    using uint40_t = SizedInteger<5, unsigned>;
    using int40_t = SizedInteger<5, signed>;

    using uint48_t = SizedInteger<6, unsigned>;
    using int48_t = SizedInteger<6, signed>;

    using uint56_t = SizedInteger<7, unsigned>;
    using int56_t = SizedInteger<7, signed>;
}
