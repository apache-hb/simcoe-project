#pragma once

namespace sm::math {
    template<typename T>
    constexpr inline T kPi = T(3.14159265358979323846264338327950288);

    template<typename T>
    constexpr inline T kRadToDeg = T(180) / kPi<T>;

    template<typename T>
    constexpr inline T kDegToRad = kPi<T> / T(180);

    template<typename T> struct Radians;
    template<typename T> struct Degrees;

    template<typename T>
    constexpr T to_radians(T degrees) {
        return degrees * kDegToRad<T>;
    }

    template<typename T>
    constexpr T to_degrees(T radians) {
        return radians * kRadToDeg<T>;
    }

    template<typename T>
    struct Radians {
        using Degrees = Degrees<T>;
        using Type = T;

        T value;

        constexpr Radians() = default;

        explicit constexpr Radians(T v) : value(v) { }
        constexpr Radians(Degrees deg) : value(to_radians(deg.value)) { }

        constexpr Degrees to_degrees() const {
            return Degrees(to_degrees(value));
        }

        constexpr Radians operator+(Radians other) const { return Radians(value + other.value); }
        constexpr Radians operator-(Radians other) const { return Radians(value - other.value); }

        constexpr Radians& operator+=(Radians other) { value += other.value; return *this; }
        constexpr Radians& operator-=(Radians other) { value -= other.value; return *this; }

        constexpr Radians operator-() const { return Radians(-value); }
    };

    template<typename T>
    struct Degrees {
        using Radians = Radians<T>;
        using Type = T;

        T value;

        constexpr Degrees() = default;

        explicit constexpr Degrees(T v) : value(v) { }
        constexpr Degrees(Radians rad) : value(to_degrees(rad.value)) { }

        constexpr Radians to_radians() const {
            return Radians(to_radians(value));
        }

        constexpr Degrees operator+(Degrees other) const { return Degrees(value + other.value); }
        constexpr Degrees operator-(Degrees other) const { return Degrees(value - other.value); }

        constexpr Degrees& operator+=(Degrees other) { value += other.value; return *this; }
        constexpr Degrees& operator-=(Degrees other) { value -= other.value; return *this; }

        constexpr Degrees operator-() const { return Degrees(-value); }
    };

    // NOLINTBEGIN
    using rad = Radians<float>;
    using radx = Radians<double>;

    using deg = Degrees<float>;
    using degx = Degrees<double>;
    // NOLINTEND
}
