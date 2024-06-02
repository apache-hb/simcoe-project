#pragma once

#include "core/core.hpp"

#include <cmath>
#include <numbers>
#include <iterator>

namespace sm::math {
    ///
    /// math consts
    ///

    template<typename T>
    constexpr inline T kPi = T(std::numbers::pi);

    template<typename T>
    constexpr inline T kRadToDeg = T(180) / kPi<T>;

    template<typename T>
    constexpr inline T kDegToRad = kPi<T> / T(180);

    template<typename T = float>
    consteval T pi() { return kPi<T>; }

    template<typename T = float>
    consteval T pi2() { return kPi<T> * T(2); }

    template<typename T = float>
    consteval T pidiv2() { return kPi<T> / T(2); }

    ///
    /// forward declarations
    ///

    template<typename T> struct Radians;
    template<typename T> struct Degrees;

    template<typename T> struct Rect;

    template<typename T> struct Vec2;
    template<typename T> struct Vec3;
    template<typename T> struct Vec4;

    template<typename T> struct Mat4x4;

    template<typename T, size_t N>
    struct Vec;

    template<typename T>
    struct Vec<T, 2> : public Vec2<T> {
        using Vec2<T>::Vec2;
    };

    template<typename T>
    struct Vec<T, 3> : public Vec3<T> {
        using Vec3<T>::Vec3;
    };

    template<typename T>
    struct Vec<T, 4> : public Vec4<T> {
        using Vec4<T>::Vec4;
    };

    ///
    /// concepts
    ///

    template<typename T>
    concept IsVector = requires (T it) {
        // exports types
        typename T::Type;

        { T::kSize } -> std::convertible_to<size_t>;

        { it.fields } -> std::convertible_to<typename T::Type*>;
        std::size(it.fields) == T::kSize;

        { it.data() } -> std::convertible_to<typename T::Type*>;
    };

    template<typename T>
    concept IsAngle = requires (T it) {
        typename T::Type;

        { it.to_degrees() } -> std::convertible_to<typename T::Degrees>;
        { it.to_radians() } -> std::convertible_to<typename T::Radians>;
        { it.get_degrees() } -> std::convertible_to<typename T::Type>;
        { it.get_radians() } -> std::convertible_to<typename T::Type>;
        { it.get() } -> std::convertible_to<typename T::Type>;
    };

    template<typename T>
    concept IsMatrix = requires (T it) {
        typename T::Type;

        { T::kRowCount } -> std::convertible_to<size_t>;
        { T::kColumnCount } -> std::convertible_to<size_t>;
        { T::kSize } -> std::convertible_to<size_t>;

        { it.fields } -> std::convertible_to<typename T::Type*>;
        std::size(it.fields) == T::kSize;

        { it.data() } -> std::convertible_to<typename T::Type*>;
    };

    ///
    /// util functions
    ///

    template<typename T>
    constexpr T clamp(T it, T low, T high) {
        if (it < low)
            return low;

        if (it > high)
            return high;

        return it;
    }

    template<IsVector T>
    constexpr T clamp(T it, T low, T high) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = clamp(it.fields[i], low.fields[i], high.fields[i]);
        }
        return out;
    }

    template<typename T>
    constexpr T min(const T& lhs, const T& rhs) {
        return lhs < rhs ? lhs : rhs;
    }

    template<IsVector T>
    constexpr T min(const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = math::min(lhs.fields[i], rhs.fields[i]);
        }
        return out;
    }

    template<typename T>
    constexpr T max(const T& lhs, const T& rhs) {
        return lhs >= rhs ? lhs : rhs;
    }

    template<IsVector T>
    constexpr T max(const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = max(lhs.fields[i], rhs.fields[i]);
        }
        return out;
    }

    template<typename T>
    constexpr bool nearly_equal(T lhs, T rhs, T epsilon = T(1e-5)) {
        return std::abs(lhs - rhs) < epsilon;
    }

    template<typename T>
    constexpr T fmod(T x, T y) {
        return std::fmod(x, y);
    }

    template<IsVector T>
    constexpr T fmod(const T& x, const T& y) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = fmod(x.fields[i], y.fields[i]);
        }
        return out;
    }

    template<typename T>
    constexpr T gather(Vec<uint, T::kSize> offsets, const typename T::Type *values) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = values[offsets.fields[i]];
        }
        return out;
    }
}
