#pragma once

#include "core/core.hpp"

#include "units.hpp"

namespace sm::math {
    template<typename T>
    concept IsVector = requires (T it) {
        { T::kSize } -> std::convertible_to<size_t>;
        { it.fields } -> std::convertible_to<typename T::Type*>;
    };

    template<typename T>
    constexpr T clamp(T it, T low, T high) {
        if (it < low)
            return low;

        if (it > high)
            return high;

        return it;
    }

    template<typename T>
    constexpr T min(const T& lhs, const T& rhs) {
        return lhs < rhs ? lhs : rhs;
    }

    template<typename T>
    constexpr T max(const T& lhs, const T& rhs) {
        return lhs > rhs ? lhs : rhs;
    }

    template<typename T>
    constexpr bool nearly_equal(T lhs, T rhs, T epsilon = T(1e-5)) {
        return std::abs(lhs - rhs) < epsilon;
    }

    template<typename T>
    struct SinCos { T sin; T cos; };

    // TODO: do an approximation to get both at once
    template<typename T>
    constexpr SinCos<T> sincos(T angle) {
        return {std::sin(angle), std::cos(angle)};
    }

    template<typename T>
    constexpr T fmod(T x, T y) {
        return std::fmod(x, y);
    }

    /**
     * vector types are (sort of) organized as follows:
     * all vectors are default uninitialized to satisfy std::is_trivial
     * struct Type {
     *    fields...
     *
     *    constructors
     *    comparison operators
     *    arithmetic operators
     *    in place arithmetic operators
     *    conversion operators
     *    vector specific functions
     * }
     */

    template<typename T> struct Vec2;
    template<typename T> struct Vec3;
    template<typename T> struct Vec4;

    template<typename T> struct Rect;
    template<typename T> struct Quat;

    template<typename T> struct Mat4x4;

    template<typename T>
    struct alignas(sizeof(T) * 2) Vec2 {
        static constexpr size_t kSize = 2;
        using Type = T;

        union {
            T fields[2];
            struct { T x; T y; };
            struct { T u; T v; };
            struct { T width; T height; };
        };

        constexpr Vec2() = default;

        constexpr Vec2(T x, T y) : x(x), y(y) { }
        constexpr Vec2(T it) : Vec2(it, it) { }
        constexpr Vec2(const T *data) : Vec2(data[0], data[1]) { }

        constexpr static Vec2 zero() { return Vec2(T(0)); }

        constexpr bool operator==(const Vec2& other) const { return x == other.x && y == other.y; }
        constexpr bool operator!=(const Vec2& other) const { return x != other.x || y != other.y; }

        constexpr Vec2 operator-() const { return neg(); }
        constexpr Vec2 operator+() const { return abs(); }

        constexpr Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
        constexpr Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
        constexpr Vec2 operator*(const Vec2& other) const { return Vec2(x * other.x, y * other.y); }
        constexpr Vec2 operator/(const Vec2& other) const { return Vec2(x / other.x, y / other.y); }

        constexpr Vec2 operator+=(const Vec2& other) { return *this = *this + other; }
        constexpr Vec2 operator-=(const Vec2& other) { return *this = *this - other; }
        constexpr Vec2 operator*=(const Vec2& other) { return *this = *this * other; }
        constexpr Vec2 operator/=(const Vec2& other) { return *this = *this / other; }

        template<typename O>
        constexpr Vec2<O> as() const { return Vec2<O>(O(x), O(y)); }

        bool isinf() const { return std::isinf(x) || std::isinf(y); }

        constexpr bool is_uniform() const { return x == y; }

        constexpr Vec2 neg() const { return Vec2(-x, -y); }
        constexpr Vec2 abs() const { return Vec2(std::abs(x), std::abs(y)); }
        constexpr T length() const { return std::sqrt(x * x + y * y); }

        constexpr Vec2 normalized() const {
            auto len = length();
            return Vec2(x / len, y / len);
        }

        template<typename O>
        constexpr Vec2<O> floor() const { return Vec2<O>(O(std::floor(x)), O(std::floor(y))); }

        template<typename O>
        constexpr Vec2<O> ceil() const { return Vec2<O>(O(std::ceil(x)), O(std::ceil(y))); }

        constexpr Vec2 clamp(const Vec2& low, const Vec2& high) const {
            return Vec2(
                math::clamp(x, low.x, high.x),
                math::clamp(y, low.y, high.y)
            );
        }

        template<typename R>
        constexpr Vec2 rotate(R angle, const Vec2& origin = Vec2{}) const {
            auto [x, y] = *this - origin;

            auto sin = std::sin(to_radians(angle));
            auto cos = std::cos(to_radians(angle));

            auto x1 = x * cos - y * sin;
            auto y1 = x * sin + y * cos;

            return Vec2(x1, y1) + origin;
        }

        static constexpr T dot(const Vec2& lhs, const Vec2& rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y;
        }

        constexpr const T& operator[](size_t index) const { return at(index); }
        constexpr T& operator[](size_t index) { return at(index); }

        constexpr const T& at(size_t index) const { verify_index(index); return fields[index];}
        constexpr T& at(size_t index) { verify_index(index); return fields[index]; }

        constexpr T *data() { return fields; }
        constexpr const T *data() const { return fields; }

        template<size_t I>
        constexpr decltype(auto) get() const {
            if constexpr (I == 0) return x;
            else if constexpr (I == 1) return y;
            else static_assert(I < 2, "index out of bounds");
        }

        constexpr void verify_index([[maybe_unused]] size_t index) const {
            CTASSERTF(index < 2, "index out of bounds (%zu < 2)", index);
        }
    };

    template<typename T>
    struct Vec3 {
        static constexpr size_t kSize = 3;
        using Vec2 = Vec2<T>;
        using Type = T;

        union {
            T fields[3];
            struct { T x; T y; T z; };
            struct { T r; T g; T b; };
            struct { T roll; T pitch; T yaw; };
        };

        constexpr Vec3() = default;

        constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) { }
        constexpr Vec3(T it) : Vec3(it, it, it){ }
        constexpr Vec3(const Vec2& xy, T z) : Vec3(xy.x, xy.y, z) { }
        constexpr Vec3(T x, const Vec2& yz) : Vec3(x, yz.x, yz.y) { }
        constexpr Vec3(const T *data) : Vec3(data[0], data[1], data[2]) { }

        static constexpr Vec3 zero() { return Vec3(T(0)); }

        constexpr bool operator==(const Vec3& other) const { return x == other.x && y == other.y && z == other.z; }
        constexpr bool operator!=(const Vec3& it) const { return x != it.x || y != it.y || z != it.z; }

        constexpr Vec3 operator+(const Vec3& it) const { return Vec3(x + it.x, y + it.y, z + it.z); }
        constexpr Vec3 operator-(const Vec3& it) const { return Vec3(x - it.x, y - it.y, z - it.z); }
        constexpr Vec3 operator*(const Vec3& it) const { return Vec3(x * it.x, y * it.y, z * it.z); }
        constexpr Vec3 operator/(const Vec3& it) const { return Vec3(x / it.x, y / it.y, z / it.z); }

        constexpr Vec3 operator+=(const Vec3& it) { return *this = *this + it; }
        constexpr Vec3 operator-=(const Vec3& it) { return *this = *this - it; }
        constexpr Vec3 operator*=(const Vec3& it) { return *this = *this * it; }
        constexpr Vec3 operator/=(const Vec3& it) { return *this = *this / it; }

        constexpr Vec3 operator-() const { return negate(); }
        constexpr Vec3 operator+() const { return abs(); }

        template<typename O>
        constexpr Vec3<O> as() const { return Vec3<O>(O(x), O(y), O(z)); }

        constexpr Vec2 xy() const { return Vec2(x, y); }
        constexpr Vec2 xz() const { return Vec2(x, z); }
        constexpr Vec2 yz() const { return Vec2(y, z); }

        bool isinf() const { return std::isinf(x) || std::isinf(y) || std::isinf(z); }

        constexpr bool is_uniform() const { return x == y && y == z; }

        constexpr Vec3 negate() const { return Vec3(-x, -y, -z); }
        constexpr Vec3 abs() const { return Vec3(std::abs(x), std::abs(y), std::abs(z)); }
        constexpr T length() const { return std::sqrt(x * x + y * y + z * z); }

        constexpr Vec3 normalized() const {
            auto len = length();
            return Vec3(x / len, y / len, z / len);
        }

        constexpr const T& operator[](size_t index) const { return at(index); }
        constexpr T& operator[](size_t index) { return at(index); }

        constexpr const T& at(size_t index) const { verify_index(index); return fields[index];}
        constexpr T& at(size_t index) { verify_index(index); return fields[index]; }

        constexpr Vec3 clamp(const Vec3& low, const Vec3& high) const {
            return Vec3(
                math::clamp(x, low.x, high.x),
                math::clamp(y, low.y, high.y),
                math::clamp(z, low.z, high.z)
            );
        }

        static constexpr Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
            return Vec3(
                lhs.y * rhs.z - lhs.z * rhs.y,
                lhs.z * rhs.x - lhs.x * rhs.z,
                lhs.x * rhs.y - lhs.y * rhs.x
            );
        }

        static constexpr T dot(const Vec3& lhs, const Vec3& rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        static constexpr Vec3 rotate(const Vec3& point, const Vec3& origin, const Vec3& rotate) {
            auto [x, y, z] = point - origin;

            auto sinX = std::sin(rotate.x);
            auto cosX = std::cos(rotate.x);

            auto sinY = std::sin(rotate.y);
            auto cosY = std::cos(rotate.y);

            auto sinZ = std::sin(rotate.z);
            auto cosZ = std::cos(rotate.z);

            auto x1 = x * cosZ - y * sinZ;
            auto y1 = x * sinZ + y * cosZ;

            auto x2 = x1 * cosY + z * sinY;
            auto z2 = x1 * -sinY + z * cosY;

            auto y3 = y1 * cosX - z2 * sinX;
            auto z3 = y1 * sinX + z2 * cosX;

            return Vec3(x2, y3, z3) + origin;
        }

        constexpr T *data() { return fields; }
        constexpr const T *data() const { return fields; }

        template<size_t I>
        constexpr decltype(auto) get() const {
            if constexpr (I == 0) return x;
            else if constexpr (I == 1) return y;
            else if constexpr (I == 2) return z;
            else static_assert(I < 3, "index out of bounds");
        }

        constexpr void verify_index([[maybe_unused]] size_t index) const {
            CTASSERTF(index < 3, "index out of bounds (%zu < 3)", index);
        }
    };

    template<typename T>
    struct alignas(sizeof(T) * 4) Vec4 {
        static constexpr size_t kSize = 4;
        using Vec2 = Vec2<T>;
        using Vec3 = Vec3<T>;
        using Type = T;

        union {
            T fields[4];
            struct { T x; T y; T z; T w; };
            struct { T r; T g; T b; T a; };
        };

        constexpr Vec4() = default;

        constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
        constexpr Vec4(T it) : Vec4(it, it, it, it) { }
        constexpr Vec4(const Vec3& xyz, T w) : Vec4(xyz.x, xyz.y, xyz.z, w) { }
        constexpr Vec4(const T *data) : Vec4(data[0], data[1], data[2], data[3]) { }

        static constexpr Vec4 zero() { return Vec4(T(0)); }

        constexpr bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
        constexpr bool operator!=(const Vec4& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }

        constexpr Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
        constexpr Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
        constexpr Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
        constexpr Vec4 operator/(const Vec4& other) const { return Vec4(x / other.x, y / other.y, z / other.z, w / other.w); }

        constexpr Vec4 operator+=(const Vec4& other) { return *this = *this + other; }
        constexpr Vec4 operator-=(const Vec4& other) { return *this = *this - other; }
        constexpr Vec4 operator*=(const Vec4& other) { return *this = *this * other; }
        constexpr Vec4 operator/=(const Vec4& other) { return *this = *this / other; }

        template<typename O>
        constexpr Vec4<O> as() const { return Vec4<O>(O(x), O(y), O(z), O(w)); }

        constexpr Vec3 xyz() const { return Vec3(x, y, z); }

        bool isinf() const { return std::isinf(x) || std::isinf(y) || std::isinf(z) || std::isinf(w); }
        constexpr bool is_uniform() const { return x == y && y == z && z == w; }

        constexpr T length() const { return std::sqrt(x * x + y * y + z * z + w * w); }
        constexpr Vec4 negate() const { return Vec4(-x, -y, -z, -w); }

        constexpr Vec4 normalized() const {
            auto len = length();
            return Vec4(x / len, y / len, z / len, w / len);
        }

        constexpr Vec4 clamp(const Vec4& low, const Vec4& high) const {
            return Vec4(
                math::clamp(x, low.x, high.x),
                math::clamp(y, low.y, high.y),
                math::clamp(z, low.z, high.z),
                math::clamp(w, low.w, high.w)
            );
        }

        constexpr const T& operator[](size_t index) const { return at(index); }
        constexpr T& operator[](size_t index) { return at(index); }

        constexpr const T& at(size_t index) const { verify_index(index); return fields[index];}
        constexpr T& at(size_t index) { verify_index(index); return fields[index]; }

        constexpr T *data() { return fields; }
        constexpr const T *data() const { return fields; }

        template<size_t I>
        constexpr decltype(auto) get() const {
            if constexpr (I == 0) return x;
            else if constexpr (I == 1) return y;
            else if constexpr (I == 2) return z;
            else if constexpr (I == 3) return w;
            else static_assert(I < 4, "index out of bounds");
        }

        constexpr void verify_index([[maybe_unused]] size_t index) const {
            CTASSERTF(index < 4, "index out of bounds (%zu < 4)", index);
        }
    };

    template <typename T>
    struct alignas(sizeof(T) * 4) Rect {
        using Vec2 = Vec2<T>;
        using Vec4 = Vec4<T>;

        struct Members {
            T left;
            T top;
            T right;
            T bottom;
        };

        union {
            T fields[4];
            struct { T left; T top; T right; T bottom; };
        };

        constexpr Rect() = default;
        constexpr Rect(T left, T top, T right, T bottom)
            : left(left)
            , top(top)
            , right(right)
            , bottom(bottom)
        { }

        constexpr Rect(Members members)
            : Rect(members.left, members.top, members.right, members.bottom)
        { }

        constexpr Rect(const Vec2 &pos, const Vec2 &size)
            : Rect(pos.x, pos.y, pos.x + size.x, pos.y + size.y)
        { }

        constexpr Vec2 position() const { return Vec2(left, top); }
        constexpr Vec2 size() const { return Vec2(right - left, bottom - top); }

        constexpr bool operator==(const Rect &other) const {
            return left == other.left
                && top == other.top
                && right == other.right
                && bottom == other.bottom;
        }

        constexpr bool operator!=(const Rect &other) const {
            return left != other.left
                || top != other.top
                || right != other.right
                || bottom != other.bottom;
        }

        constexpr T area() const {
            auto [w, h] = size();
            return w * h;
        }

        template<size_t I>
        constexpr decltype(auto) get() const {
            if constexpr (I == 0) return left;
            else if constexpr (I == 1) return top;
            else if constexpr (I == 2) return right;
            else if constexpr (I == 3) return bottom;
            else static_assert(I < 4, "index out of bounds");
        }

        constexpr T *data() { return fields; }
        constexpr const T *data() const { return fields; }
    };

    // TODO: this doesnt work, need to do some actual math studying
    template <typename T>
    struct alignas(sizeof(T) * 4) Quat {
        using Vec3 = Vec3<T>;
        using Vec4 = Vec4<T>;
        using Mat4x4 = Mat4x4<T>;

        Vec3 v;
        T angle;

        constexpr Quat() = default;
        constexpr Quat(const Vec3 &v, T w)
            : v(v)
            , angle(w)
        { }

        constexpr Quat(T x, T y, T z, T w)
            : v(x, y, z)
            , angle(w)
        { }

        static constexpr Quat identity() {
            return {Vec3::zero(), 1};
        }

        constexpr Quat conjugate() const {
            return {-v, angle};
        }

#if 0
        constexpr Vec3 as_euler() const {
            T sinr_cosp = T(2) * (angle * v.x + v.y * v.z);
            T cosr_cosp = T(1) - T(2) * (v.x * v.x + v.y * v.y);
            T roll = std::atan2(sinr_cosp, cosr_cosp);

            T sinp = std::sqrt(T(1) + T(2) * (angle * v.y - v.x * v.z));
            T cosp = std::sqrt(T(1) - T(2) * (angle * v.y - v.x * v.z));
            T pitch = 2 * std::atan2(sinp, cosp) - kPi<T> / 2;

            T siny_cosp = T(2) * (angle * v.z + v.x * v.y);
            T cosy_cosp = T(1) - T(2) * (v.y * v.y + v.z * v.z);
            T yaw = std::atan2(siny_cosp, cosy_cosp);

            return {roll, pitch, yaw};
        }

        constexpr Vec3 as_euler2() const {
            T sqw = angle * angle;
            T sqx = v.x * v.x;
            T sqy = v.y * v.y;
            T sqz = v.z * v.z;

            T unit = sqx + sqy + sqz + sqw;
            T test = v.x * v.y + v.z * angle;
            if (test > T(0.499 * unit)) {
                T yaw = T(2) * std::atan2(v.x, angle);
                T pitch = kPi<T> / T(2);
                T roll = T(0);
                return {roll, pitch, yaw};
            } else if (test < T(-0.499 * unit)) {
                T yaw = T(-2) * std::atan2(v.x, angle);
                T pitch = -kPi<T> / T(2);
                T roll = T(0);
                return {roll, pitch, yaw};
            } else {
                T yaw = std::atan2(T(2) * v.y * angle - T(2) * v.x * v.z, sqx - sqy - sqz + sqw);
                T pitch = std::asin(T(2) * test / unit);
                T roll = std::atan2(T(2) * v.x * angle - T(2) * v.y * v.z, -sqx + sqy - sqz + sqw);
                return {roll, pitch, yaw};
            }
        }

        constexpr Quat operator*(const Quat &o) const {
            const auto [x0, y0, z0, w0] = *this;
            const auto [x1, y1, z1, w1] = o;

            const T x = (w1 * x0) + (x1 * w0) + (y1 * z0) - (z1 * y0);
            const T y = (w1 * y0) - (x1 * z0) + (y1 * w0) + (x1 * x0);
            const T z = (w1 * z0) + (x1 * y0) - (y1 * x0) + (z1 * w0);
            const T w = (w1 * w0) - (x1 * x0) - (y1 * y0) - (z1 * z0);

            return {x, y, z, w};
        }

        static constexpr Quat mul(const Quat &lhs, const Quat &rhs) {
            return lhs * rhs;
        }

        static constexpr Quat from_axes(T bank, T attitude, T heading) {
            T hr = bank * T(0.5);
            T c3 = std::cos(hr);
            T s3 = std::sin(hr);

            T hp = attitude * T(0.5);
            T c2 = std::cos(hp);
            T s2 = std::sin(hp);

            T hy = heading * T(0.5);
            T c1 = std::cos(hy);
            T s1 = std::sin(hy);

            T c1c2 = c1 * c2;
            T s1s2 = s1 * s2;

            const T w = c1c2 * c3 - s1s2 * s3;

            const T v0 = c1c2 * s3 - s1s2 * c3; // correct
            const T v1 = s1 * c2 * c3 + c1 * s2 * s3;
            const T v2 = c1 * s2 * c3 - s1 * c2 * s3;

            return {v0, v1, v2, w};
        }

        static constexpr Quat from_axes2(T roll, T pitch, T yaw) {
            auto r2 = kDegToRad<T> / T(2);

            auto pn = math::fmod(pitch, T(360));
            auto yn = math::fmod(yaw, T(360));
            auto rn = math::fmod(roll, T(360));

            auto [sp, cp] = math::sincos(pn * r2);
            auto [sy, cy] = math::sincos(yn * r2);
            auto [sr, cr] = math::sincos(rn * r2);

            T x =  cr*sp*sy - sp*cp*cy;
            T y = -cr*sp*cy - sr*cp*sy;
            T z =  cr*cp*sy - sr*sp*cy;
            T w =  cr*cp*cy + sr*sp*sy;

            return {x, y, z, w};
        }

        static constexpr Quat from_axes(const Vec3 &angles) {
            return Quat::from_axes(angles.roll, angles.pitch, angles.yaw);
        }

        static constexpr Quat from_matrix(const Mat4x4& matrix) {
            T r22 = matrix.at(2, 2);
            if (r22 <= 0) {
                T dif10 = matrix.at(1, 1) - matrix.at(0, 0);
                T omr22 = 1 - r22;
                if (dif10 <= 0) {
                    T fxsqr = omr22 - dif10;
                    T inv4x = T(0.5) / std::sqrt(fxsqr);
                    return Quat{
                        fxsqr * inv4x,
                        (matrix.at(0, 1) + matrix.at(1, 0)) * inv4x,
                        (matrix.at(0, 2) + matrix.at(2, 0)) * inv4x,
                        (matrix.at(1, 2) - matrix.at(0, 2)) * inv4x,
                    };
                }
                else {
                    T fysqr = omr22 + dif10;
                    T inv4y = T(0.5) / std::sqrt(fysqr);
                    return Quat{
                        matrix.at(0, 1) + matrix.at(1, 0) * inv4y,
                        fysqr * inv4y,
                        (matrix.at(1, 2) + matrix.at(2, 1)) * inv4y,
                        (matrix.at(2, 0) - matrix.at(0, 2)) * inv4y,
                    };
                }
            }
            else {
                T sum10 = matrix.at(1, 1) + matrix.at(0, 0);
                T opr22 = 1 + r22;
                if (sum10 <= 0) {
                    T fzsqr = opr22 - sum10;
                    T inv4z = T(0.5) / std::sqrt(fzsqr);
                    return Quat{
                        (matrix.at(0, 2) + matrix.at(2, 0)) * inv4z,
                        (matrix.at(1, 2) + matrix.at(2, 1)) * inv4z,
                        fzsqr * inv4z,
                        (matrix.at(0, 1) - matrix.at(1, 0)) * inv4z,
                    };
                }
                else {
                    T fwsqr = opr22 + sum10;
                    T inv4w = T(0.5) / std::sqrt(fwsqr);
                    return Quat{
                        (matrix.at(1, 2) - matrix.at(2, 1)) * inv4w,
                        (matrix.at(2, 0) - matrix.at(0, 2)) * inv4w,
                        (matrix.at(0, 1) - matrix.at(1, 0)) * inv4w,
                        fwsqr * inv4w,
                    };
                }
            }
        }
#endif

        constexpr void decompose(Vec3 &axis, T &angle) const {
            axis = v.normalized();
            angle = std::acos(angle) * T(2);
        }
    };

    template<typename T>
    struct alignas(sizeof(T) * 16) Mat4x4 {
        using Type = T;
        using Rad = Radians<T>;
        using Rad3 = Vec3<Rad>;
        using Vec4 = Vec4<T>;
        using Vec3 = Vec3<T>;
        using Quat = Quat<T>;

        union {
            T fields[16];
            T matrix[4][4];
            Vec4 rows[4];
        };

        constexpr Mat4x4() = default;

        constexpr Mat4x4(T it) : Mat4x4(Vec4(it)) { }
        constexpr Mat4x4(const Vec4& row) : Mat4x4(row, row, row, row) { }
        constexpr Mat4x4(const Vec4& row0, const Vec4& row1, const Vec4& row2, const Vec4& row3) : rows{ row0, row1, row2, row3 } { }
        constexpr Mat4x4(const T *data) : Mat4x4(Vec4(data), Vec4(data + 4), Vec4(data + 8), Vec4(data + 12)) { }

        constexpr Vec4 column(size_t column) const {
            return { at(column, 0), at(column, 1), at(column, 2), at(column, 3) };
        }

        constexpr Vec4 row(size_t row) const { return at(row); }

        constexpr const Vec4& at(size_t it) const { return rows[it]; }
        constexpr Vec4& at(size_t it) { return rows[it]; }

        constexpr const Vec4& operator[](size_t row) const { return rows[row];}
        constexpr Vec4& operator[](size_t row) { return rows[row]; }

        constexpr const T &at(size_t it, size_t col) const { return at(it).at(col); }
        constexpr T &at(size_t it, size_t col) { return at(it).at(col); }

        constexpr T *data() { return fields; }
        constexpr const T *data() const { return fields; }

        constexpr Vec4 mul(const Vec4& other) const {
            auto row0 = at(0);
            auto row1 = at(1);
            auto row2 = at(2);
            auto row3 = at(3);

            auto x = row0.x * other.x + row0.y * other.y + row0.z * other.z + row0.w * other.w;
            auto y = row1.x * other.x + row1.y * other.y + row1.z * other.z + row1.w * other.w;
            auto z = row2.x * other.x + row2.y * other.y + row2.z * other.z + row2.w * other.w;
            auto w = row3.x * other.x + row3.y * other.y + row3.z * other.z + row3.w * other.w;

            return { x, y, z, w };
        }

        constexpr Mat4x4 mul(const Mat4x4& other) const {
            auto row0 = at(0);
            auto row1 = at(1);
            auto row2 = at(2);
            auto row3 = at(3);

            auto other0 = other.at(0);
            auto other1 = other.at(1);
            auto other2 = other.at(2);
            auto other3 = other.at(3);

            Vec4 out0 = {
                (other0.x * row0.x) + (other1.x * row0.y) + (other2.x * row0.z) + (other3.x * row0.w),
                (other0.y * row0.x) + (other1.y * row0.y) + (other2.y * row0.z) + (other3.y * row0.w),
                (other0.z * row0.x) + (other1.z * row0.y) + (other2.z * row0.z) + (other3.z * row0.w),
                (other0.w * row0.x) + (other1.w * row0.y) + (other2.w * row0.z) + (other3.w * row0.w)
            };

            Vec4 out1 = {
                (other0.x * row1.x) + (other1.x * row1.y) + (other2.x * row1.z) + (other3.x * row1.w),
                (other0.y * row1.x) + (other1.y * row1.y) + (other2.y * row1.z) + (other3.y * row1.w),
                (other0.z * row1.x) + (other1.z * row1.y) + (other2.z * row1.z) + (other3.z * row1.w),
                (other0.w * row1.x) + (other1.w * row1.y) + (other2.w * row1.z) + (other3.w * row1.w)
            };

            Vec4 out2 = {
                (other0.x * row2.x) + (other1.x * row2.y) + (other2.x * row2.z) + (other3.x * row2.w),
                (other0.y * row2.x) + (other1.y * row2.y) + (other2.y * row2.z) + (other3.y * row2.w),
                (other0.z * row2.x) + (other1.z * row2.y) + (other2.z * row2.z) + (other3.z * row2.w),
                (other0.w * row2.x) + (other1.w * row2.y) + (other2.w * row2.z) + (other3.w * row2.w)
            };

            Vec4 out3 = {
                (other0.x * row3.x) + (other1.x * row3.y) + (other2.x * row3.z) + (other3.x * row3.w),
                (other0.y * row3.x) + (other1.y * row3.y) + (other2.y * row3.z) + (other3.y * row3.w),
                (other0.z * row3.x) + (other1.z * row3.y) + (other2.z * row3.z) + (other3.z * row3.w),
                (other0.w * row3.x) + (other1.w * row3.y) + (other2.w * row3.z) + (other3.w * row3.w)
            };

            return { out0, out1, out2, out3 };
        }

        constexpr Mat4x4 add(const Mat4x4& other) const {
            return {
                at(0).add(other.at(0)),
                at(1).add(other.at(1)),
                at(2).add(other.at(2)),
                at(3).add(other.at(3))
            };
        }

        constexpr Mat4x4 operator*(const Mat4x4& other) const {
            return mul(other);
        }

        constexpr Mat4x4 operator*=(const Mat4x4& other) {
            return *this = *this * other;
        }

        constexpr Mat4x4 operator+(const Mat4x4& other) const {
            return add(other);
        }

        constexpr Mat4x4 operator+=(const Mat4x4& other) {
            return *this = *this + other;
        }

        ///
        /// scale related functions
        ///

        static constexpr Mat4x4 scale(const Vec3& scale) {
            return Mat4x4::scale(scale.x, scale.y, scale.z);
        }

        static constexpr Mat4x4 scale(T x, T y, T z) {
            Vec4 row0 = { x, 0, 0, 0 };
            Vec4 row1 = { 0, y, 0, 0 };
            Vec4 row2 = { 0, 0, z, 0 };
            Vec4 row3 = { 0, 0, 0, 1 };
            return { row0, row1, row2, row3 };
        }

        constexpr Vec3 get_scale() const {
            return { at(0, 0), at(1, 1), at(2, 2) };
        }

        constexpr void set_scale(const Vec3& scale) {
            at(0, 0) = scale.x;
            at(1, 1) = scale.y;
            at(2, 2) = scale.z;
        }

        ///
        /// translation related functions
        ///

        static constexpr Mat4x4 translation(const Vec3& translation) {
            return Mat4x4::translation(translation.x, translation.y, translation.z);
        }

        static constexpr Mat4x4 translation(T x, T y, T z) {
            Vec4 row0 = { 1, 0, 0, x };
            Vec4 row1 = { 0, 1, 0, y };
            Vec4 row2 = { 0, 0, 1, z };
            Vec4 row3 = { 0, 0, 0, 1 };
            return { row0, row1, row2, row3 };
        }

        constexpr Vec3 get_translation() const {
            return { at(0, 3), at(1, 3), at(2, 3) };
        }

        constexpr void set_translation(const Vec3& translation) {
            at(0, 3) = translation.x;
            at(1, 3) = translation.y;
            at(2, 3) = translation.z;
        }

        // rotation related functions

        /// @brief creates a rotation matrix from a vector
        /// @note the vector is expected to be in radians
        /// @param rotation the rotation vector
        /// @return the rotation matrix
        static constexpr Mat4x4 rotation(const Vec3& rotation) {
            auto [sr, cr] = math::sincos(rotation.roll);
            auto [sy, cy] = math::sincos(rotation.yaw);
            auto [sp, cp] = math::sincos(rotation.pitch);

            Vec4 r0 = {
                cr * cy + sr * sp * sy,
                sr * cp,
                sr * sp * cy - cr * sy,
                0
            };

            Vec4 r1 = {
                cr * sp * sy - sr * cy,
                cr * cp,
                sr * sy + cr * sp * cy,
                0
            };

            Vec4 r2 = {
                cp * sy,
                -sp,
                cp * cy,
                0
            };

            Vec4 r3 = { 0, 0, 0, 1 };

            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 rotation(const Quat& q) {
            // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/jay.htm
            auto [x, y, z] = q.v;
            auto w = q.angle;

            Vec4 x0 = { w, z, -y, x };
            Vec4 y0 = { -z, w, x, y };
            Vec4 z0 = { y, -x, w, z };
            Vec4 w0 = { -x, -y, -z, w };

            Vec4 x1 = { w, z, -y, -x };
            Vec4 y1 = { -z, w, x, -y };
            Vec4 z1 = { y, -x, w, -z };
            Vec4 w1 = { x, y, z, w };

            Mat4x4 lhs = { x0, y0, z0, w0 };
            Mat4x4 rhs = { x1, y1, z1, w1 };

            return lhs * rhs;
        }

        // full transform
        static constexpr Mat4x4 transform(const Vec3& translation, const Vec3& rotation, const Vec3& scale) {
            return Mat4x4::translation(translation) * Mat4x4::rotation(rotation) * Mat4x4::scale(scale);
        }

        static constexpr Mat4x4 transform(const Vec3& translation, const Quat& rotation, const Vec3& scale) {
            return Mat4x4::translation(translation) * Mat4x4::rotation(rotation) * Mat4x4::scale(scale);
        }

        constexpr Mat4x4 transpose() const {
            Vec4 r0 = { rows[0].x, rows[1].x, rows[2].x, rows[3].x };
            Vec4 r1 = { rows[0].y, rows[1].y, rows[2].y, rows[3].y };
            Vec4 r2 = { rows[0].z, rows[1].z, rows[2].z, rows[3].z };
            Vec4 r3 = { rows[0].w, rows[1].w, rows[2].w, rows[3].w };
            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 identity() {
            Vec4 row0 = { 1, 0, 0, 0 };
            Vec4 row1 = { 0, 1, 0, 0 };
            Vec4 row2 = { 0, 0, 1, 0 };
            Vec4 row3 = { 0, 0, 0, 1 };
            return { row0, row1, row2, row3 };
        }

        // camera related functions

        static constexpr Mat4x4 lookToLH(const Vec3& eye, const Vec3& dir, const Vec3& up) {
            CTASSERT(eye != Vec3::zero());
            CTASSERT(up != Vec3::zero());

            CTASSERT(!eye.isinf());
            CTASSERT(!up.isinf());

            auto r2 = dir.normalized();
            auto r0 = Vec3::cross(up, r2).normalized();
            auto r1 = Vec3::cross(r2, r0);

            auto negEye = eye.negate();

            auto d0 = Vec3::dot(r0, negEye);
            auto d1 = Vec3::dot(r1, negEye);
            auto d2 = Vec3::dot(r2, negEye);

            Vec4 s0 = { r0, d0 };
            Vec4 s1 = { r1, d1 };
            Vec4 s2 = { r2, d2 };
            Vec4 s3 = { 0, 0, 0, 1 };

            return Mat4x4(s0, s1, s2, s3).transpose();
        }

        static constexpr Mat4x4 lookToRH(const Vec3& eye, const Vec3& dir, const Vec3& up) {
            return Mat4x4::lookToLH(eye, dir.negate(), up);
        }

        static constexpr Mat4x4 lookAtRH(const Vec3& eye, const Vec3& focus, const Vec3& up) {
            return Mat4x4::lookToLH(eye, eye - focus, up);
        }

        static constexpr Mat4x4 perspectiveRH(Rad fov, T aspect, T nearLimit, T farLimit) {
            auto fovSin = math::sin(fov / 2);
            auto fovCos = math::cos(fov / 2);

            auto height = fovCos / fovSin;
            auto width = height / aspect;
            auto range = farLimit / (nearLimit - farLimit);

            Vec4 r0 = { width, 0,      0,                 0 };
            Vec4 r1 = { 0,     height, 0,                 0 };
            Vec4 r2 = { 0,     0,      range,            -1 };
            Vec4 r3 = { 0,     0,      range * nearLimit, 0 };
            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 orthographicRH(T width, T height, T nearLimit, T farLimit) {
            T range = 1 / (nearLimit - farLimit);

            Vec4 r0 = { 2 / width, 0,          0,                 0 };
            Vec4 r1 = { 0,         2 / height, 0,                 0 };
            Vec4 r2 = { 0,         0,          range,             0 };
            Vec4 r3 = { 0,         0,          range * nearLimit, 1 };

            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 orthographicRH(T left, T right, T top, T bottom, T nearLimit, T farLimit) {
            T range = 1 / (nearLimit - farLimit);

            T rWidth = 1.f / (right - left);
            T rHeight = 1.f / (top - bottom);

            Vec4 r0 = { rWidth + rWidth, 0.f, 0.f, 0.f };
            Vec4 r1 = { 0.f, rHeight + rHeight, 0.f, 0.f };
            Vec4 r2 = { 0.f, 0.f, range, 0.f };

            Vec4 r3 = {
                -(left + right) * rWidth,
                -(top + bottom) * rHeight,
                range * nearLimit,
                1.f
            };

            return { r0, r1, r2, r3 };
        }
    };

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

    // NOLINTEND

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

    // NOLINTBEGIN
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
    constexpr T min(const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = math::min(lhs.fields[i], rhs.fields[i]);
        }
        return out;
    }

    template<IsVector T>
    constexpr T max(const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = math::max(lhs.fields[i], rhs.fields[i]);
        }
        return out;
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
