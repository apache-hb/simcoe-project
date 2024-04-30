#pragma once

#include "core/core.hpp"

#include "math/utils.hpp"
#include "math/vector.hpp"
#include "math/quat.hpp"
#include "math/rect.hpp"
#include "math/matrix.hpp"

#include "math/units.hpp"

namespace sm::math {
    // NOLINTBEGIN
    using quatf = Quat<float>;
    using quatd = Quat<double>;
    static_assert(sizeof(quatf) == sizeof(float) * 4);
    static_assert(sizeof(quatd) == sizeof(double) * 4);

    using rectf = Rect<float>;
    using rectd = Rect<double>;
    using rect32 = Rect<uint32>;
    static_assert(sizeof(rectf) == sizeof(float) * 4);
    static_assert(sizeof(rectd) == sizeof(double) * 4);
    static_assert(sizeof(rect32) == sizeof(uint32) * 4);

    using int2 = Vec2<int>;
    using int3 = Vec3<int>;
    using int4 = Vec4<int>;
    static_assert(sizeof(int2) == sizeof(int) * 2);
    static_assert(sizeof(int3) == sizeof(int) * 3);
    static_assert(sizeof(int4) == sizeof(int) * 4);

    using uint2 = Vec2<uint32>;
    using uint3 = Vec3<uint32>;
    using uint4 = Vec4<uint32>;
    static_assert(sizeof(uint2) == sizeof(uint32) * 2);
    static_assert(sizeof(uint3) == sizeof(uint32) * 3);
    static_assert(sizeof(uint4) == sizeof(uint32) * 4);

    using size2 = Vec2<size_t>;
    using size3 = Vec3<size_t>;
    using size4 = Vec4<size_t>;
    static_assert(sizeof(size2) == sizeof(size_t) * 2);
    static_assert(sizeof(size3) == sizeof(size_t) * 3);
    static_assert(sizeof(size4) == sizeof(size_t) * 4);

    using float2 = Vec2<float>;
    using float3 = Vec3<float>;
    using float4 = Vec4<float>;
    using float4x4 = Mat4x4<float>;
    static_assert(sizeof(float2) == sizeof(float) * 2);
    static_assert(sizeof(float3) == sizeof(float) * 3);
    static_assert(sizeof(float4) == sizeof(float) * 4);
    static_assert(sizeof(float4x4) == sizeof(float) * 4 * 4);

    using radf2 = Radians<float2>;
    using radf3 = Radians<float3>;
    static_assert(sizeof(radf2) == sizeof(float) * 2);
    static_assert(sizeof(radf3) == sizeof(float) * 3);

    using degf2 = Degrees<float2>;
    using degf3 = Degrees<float3>;
    static_assert(sizeof(degf2) == sizeof(float) * 2);
    static_assert(sizeof(degf3) == sizeof(float) * 3);

    using uint8x2 = Vec2<uint8>;
    using uint8x3 = Vec3<uint8>;
    using uint8x4 = Vec4<uint8>;
    static_assert(sizeof(uint8x2) == sizeof(uint8) * 2);
    static_assert(sizeof(uint8x3) == sizeof(uint8) * 3);
    static_assert(sizeof(uint8x4) == sizeof(uint8) * 4);

    using uint16x2 = Vec2<uint16>;
    using uint16x3 = Vec3<uint16>;
    using uint16x4 = Vec4<uint16>;
    static_assert(sizeof(uint16x2) == sizeof(uint16) * 2);
    static_assert(sizeof(uint16x3) == sizeof(uint16) * 3);
    static_assert(sizeof(uint16x4) == sizeof(uint16) * 4);

    using uint32x2 = Vec2<uint32>;
    using uint32x3 = Vec3<uint32>;
    using uint32x4 = Vec4<uint32>;

    static_assert(sizeof(uint32x2) == sizeof(uint32) * 2);
    static_assert(sizeof(uint32x3) == sizeof(uint32) * 3);
    static_assert(sizeof(uint32x4) == sizeof(uint32) * 4);

    enum Swizzle {
        X = 0, R = 0,
        Y = 1, G = 1,
        Z = 2, B = 2,
        W = 3, A = 3,
    };
    // NOLINTEND

    constexpr uint8 swizzle_mask(Swizzle x, Swizzle y, Swizzle z, Swizzle w) {
        return (x << 0) | (y << 2) | (z << 4) | (w << 6);
    }

    constexpr Swizzle swizzle_get(uint8 mask, Swizzle channel) {
        return static_cast<Swizzle>((mask >> (channel * 2)) & 0x3);
    }

    constexpr uint8 swizzle_set(uint8 mask, Swizzle channel, Swizzle value) {
        return (mask & ~(0x3 << (channel * 2))) | (value << (channel * 2));
    }

    // NOLINTBEGIN
    enum Select : uint {
        LO = 0,
        HI = 0xFFFFFFFF,
    };
    // NOLINTEND

    constexpr uint32x4 select_mask(Select x, Select y, Select z, Select w) {
        static_assert(sizeof(uint32) == sizeof(float), "uint32 and float must be the same size for masking to work");
        return uint32x4(x, y, z, w);
    }

    // TODO: should this be a static in Quat<T>
    template <typename T>
    constexpr Vec3<T> rotate(const Quat<T> &q, const Vec3<T> &v) {
        const Vec3<T> t = cross(q.v, v) * T(2);
        return v + t * q.angle + cross(q.v, t);
    }

    template<IsVector T>
    constexpr T swizzle(const T& v, uint8 mask) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = v.fields[(mask >> (i * 2)) & 0x3];
        }
        return out;
    }

    template<IsVector T>
    constexpr T swizzle(const T& v, Swizzle x = X, Swizzle y = Y, Swizzle z = Z, Swizzle w = W) {
        return swizzle(v, swizzle_mask(x, y, z, w));
    }

    template<IsVector T>
    constexpr bool nearly_equal(const T& lhs, const T& rhs, typename T::Type epsilon) {
        return (lhs - rhs).length() < epsilon;
    }

    template<IsVector T>
    constexpr T select(Vec<uint, T::kSize> mask, const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = (lhs.fields[i] & ~mask.fields[i]) | (rhs.fields[i] & mask.fields[i]);
        }
        return out;
    }
}

// NOLINTBEGIN

namespace std {
    template<typename T>
    struct tuple_size<sm::math::Vec2<T>> : std::integral_constant<size_t, 2> { };
    template<typename T>
    struct tuple_size<sm::math::Vec3<T>> : std::integral_constant<size_t, 3> { };
    template<typename T>
    struct tuple_size<sm::math::Vec4<T>> : std::integral_constant<size_t, 4> { };

    template<size_t I, typename T>
    struct tuple_element<I, sm::math::Vec2<T>> { using type = T; };

    template<size_t I, typename T>
    struct tuple_element<I, sm::math::Vec3<T>> { using type = T; };

    template<size_t I, typename T>
    struct tuple_element<I, sm::math::Vec4<T>> { using type = T; };

    template<typename T>
    struct tuple_size<sm::math::Rect<T>> : std::integral_constant<size_t, 4> { };

    template<size_t I, typename T>
    struct tuple_element<I, sm::math::Rect<T>> { using type = T; };
}

// NOLINTEND
