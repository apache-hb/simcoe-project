#pragma once

#include <numbers>
#include <cmath>

namespace sm::math {
    template<typename T>
    constexpr inline T kPi = T(std::numbers::pi);

    template<typename T>
    constexpr inline T kRadToDeg = T(180) / kPi<T>;

    template<typename T>
    constexpr inline T kDegToRad = kPi<T> / T(180);

    template<typename T> struct Radians;
    template<typename T> struct Degrees;

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

        constexpr Degrees& operator+=(Degrees other) { value += other.value; return *this; }
        constexpr Degrees& operator-=(Degrees other) { value -= other.value; return *this; }

        constexpr Degrees operator-() const { return Degrees(-value); }

        constexpr auto operator<=>(const Degrees&) const = default;
    };

    template<IsAngle T>
    constexpr auto sin(T angle) -> T::Type { return std::sin(angle.get_radians()); }

    template<IsAngle T>
    constexpr auto cos(T angle) -> T::Type { return std::cos(angle.get_radians()); }

    template<typename T>
    constexpr T sin(T value) { return std::sin(value); }

    template<typename T>
    constexpr T cos(T value) { return std::cos(value); }

    template<IsAngle T>
    bool isinf(T angle) { return std::isinf(angle.get_radians()); }

    template<typename T>
    bool isinf(T value) { return std::isinf(value); }

    template<IsAngle T>
    constexpr T to_radians(T degrees) {
        return degrees.to_radians();
    }

    template<typename T>
    constexpr T to_radians(T degrees) {
        return degrees * kDegToRad<T>;
    }

    template<IsAngle T>
    constexpr T to_degrees(T radians) {
        return radians.to_degrees();
    }

    template<typename T>
    constexpr T to_degrees(T radians) {
        return radians * kRadToDeg<T>;
    }

    // NOLINTBEGIN
    using radf = Radians<float>;
    using radd = Radians<double>;

    using degf = Degrees<float>;
    using degd = Degrees<double>;
    // NOLINTEND

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
