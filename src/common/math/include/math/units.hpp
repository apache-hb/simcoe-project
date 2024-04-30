#pragma once

#include "math/utils.hpp"

namespace sm::math {
    template<typename T>
    struct Radians {
        using Degrees = Degrees<T>;
        using Type = T;

        T value;

        constexpr Radians() = default;

        explicit constexpr Radians(T v) : value(v) { }
        constexpr Radians(Degrees deg) : value(deg.get_radians()) { }

        constexpr Degrees to_degrees() const { return Degrees(get_degrees()); }
        constexpr Radians to_radians() const { return *this; }

        constexpr T get_radians() const { return value; }
        constexpr T get_degrees() const { return value * kRadToDeg<T>; }

        constexpr T get() const { return value; }

        constexpr Radians operator+(Radians other) const { return Radians(value + other.value); }
        constexpr Radians operator-(Radians other) const { return Radians(value - other.value); }
        constexpr Radians operator*(T scalar) const { return Radians(value * scalar); }
        constexpr Radians operator/(T scalar) const { return Radians(value / scalar); }

        constexpr Radians& operator+=(Radians other) { value += other.value; return *this; }
        constexpr Radians& operator-=(Radians other) { value -= other.value; return *this; }
        constexpr Radians& operator*=(T scalar) { value *= scalar; return *this; }
        constexpr Radians& operator/=(T scalar) { value /= scalar; return *this; }

        constexpr Radians operator-() const { return Radians(-value); }

        constexpr auto operator<=>(const Radians&) const = default;

        constexpr auto* data() requires (IsVector<T>) { return value.data(); }
        constexpr const auto* data() const requires (IsVector<T>) { return value.data(); }

        constexpr auto& operator[](size_t index) requires (IsVector<T>) { return value[index]; }
        constexpr const auto& operator[](size_t index) const requires (IsVector<T>) { return value[index]; }
    };

    template<typename T>
    struct Degrees {
        using Radians = Radians<T>;
        using Type = T;

        T value;

        constexpr Degrees() = default;

        explicit constexpr Degrees(T v) : value(v) { }
        constexpr Degrees(Radians rad) : value(rad.get_degrees()) { }

        constexpr Radians to_radians() const { return Radians(get_radians()); }
        constexpr Degrees to_degrees() const { return *this; }

        constexpr T get_radians() const { return value * kDegToRad<T>; }
        constexpr T get_degrees() const { return value; }

        constexpr T get() const { return value; }

        constexpr Degrees operator+(Degrees other) const { return Degrees(value + other.value); }
        constexpr Degrees operator-(Degrees other) const { return Degrees(value - other.value); }
        constexpr Degrees operator*(T scalar) const { return Degrees(value * scalar); }
        constexpr Degrees operator/(T scalar) const { return Degrees(value / scalar); }

        constexpr Degrees& operator+=(Degrees other) { value += other.value; return *this; }
        constexpr Degrees& operator-=(Degrees other) { value -= other.value; return *this; }
        constexpr Degrees& operator*=(T scalar) { value *= scalar; return *this; }
        constexpr Degrees& operator/=(T scalar) { value /= scalar; return *this; }

        constexpr Degrees operator-() const { return Degrees(-value); }

        constexpr auto operator<=>(const Degrees&) const = default;

        constexpr auto* data() requires (IsVector<T>) { return value.data(); }
        constexpr const auto* data() const requires (IsVector<T>) { return value.data(); }

        constexpr auto& operator[](size_t index) requires (IsVector<T>) { return value[index]; }
        constexpr const auto& operator[](size_t index) const requires (IsVector<T>) { return value[index]; }
    };

    template<IsAngle T>
    constexpr auto sin(T angle) -> T::Type { return std::sin(angle.get_radians()); }

    template<IsAngle T>
    constexpr auto cos(T angle) -> T::Type { return std::cos(angle.get_radians()); }

    template<typename T>
    constexpr T sin(T value) { return std::sin(value); }

    template<typename T>
    constexpr T cos(T value) { return std::cos(value); }

    template<typename T>
    struct SinCos { T sin; T cos; };

    // TODO: do an approximation to get sin and cos at the same time
    template<IsAngle T>
    constexpr auto sincos(T angle) -> SinCos<typename T::Type> {
        return {math::sin(angle), math::cos(angle)};
    }

    template<typename T>
    constexpr SinCos<T> sincos(T angle) {
        return {math::sin(angle), math::cos(angle)};
    }

    template<IsAngle T>
    constexpr auto acos(T value) -> T::Type { return std::acos(value.get_radians()); }

    template<typename T>
    constexpr T acos(T value) { return std::acos(value); }

    template<IsAngle T>
    bool isinf(T angle) { return std::isinf(angle.get_radians()); }

    template<typename T>
    bool isinf(T value) { return std::isinf(value); }

    template<IsAngle T>
    constexpr auto to_radians(T angle) -> T::Type {
        return angle.get_radians();
    }

    template<typename T>
    constexpr T to_radians(T angle) {
        return angle * kDegToRad<T>;
    }

    template<IsAngle T>
    constexpr auto to_degrees(T angle) -> T::Type {
        return angle.get_degrees();
    }

    template<typename T>
    constexpr T to_degrees(T angle) {
        return angle * kRadToDeg<T>;
    }

    // NOLINTBEGIN
    using radf = Radians<float>;
    using radd = Radians<double>;

    using degf = Degrees<float>;
    using degd = Degrees<double>;
    // NOLINTEND

    static_assert(sizeof(radf) == sizeof(float));
    static_assert(sizeof(radd) == sizeof(double));
    static_assert(sizeof(degf) == sizeof(float));
    static_assert(sizeof(degd) == sizeof(double));

    namespace literals {
        constexpr radd operator""_radx(long double value) {
            return radd(static_cast<double>(value));
        }

        constexpr radf operator""_rad(long double value) {
            return radf(static_cast<float>(value));
        }

        constexpr degd operator""_degx(long double value) {
            return degd(static_cast<double>(value));
        }

        constexpr degf operator""_deg(long double value) {
            return degf(static_cast<float>(value));
        }
    }
}
