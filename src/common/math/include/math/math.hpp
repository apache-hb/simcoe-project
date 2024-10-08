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

    using bool2 = Vec2<bool>;
    using bool3 = Vec3<bool>;
    using bool4 = Vec4<bool>;
    // NOLINTEND
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
