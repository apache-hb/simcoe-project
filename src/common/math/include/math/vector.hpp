#pragma once

#include "base/panic.h"

#include "math/utils.hpp"

namespace sm::math {
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

        struct MembersAxis { T x; T y; };
        struct MembersUV { T u; T v; };
        struct MembersSize { T width; T height; };

        constexpr Vec2() = default;

        constexpr Vec2(T x, T y) : x(x), y(y) { }
        constexpr Vec2(T it) : Vec2(it, it) { }
        constexpr Vec2(const T *data) : Vec2(data[0], data[1]) { }

        template<typename O> requires (!std::is_same_v<T, O>)
        constexpr Vec2(Vec2<O> other) : Vec2((T)other.x, (T)other.y) { }

        constexpr Vec2(MembersAxis members) : Vec2(members.x, members.y) { }
        constexpr Vec2(MembersUV members) : Vec2(members.u, members.v) { }
        constexpr Vec2(MembersSize members) : Vec2(members.width, members.height) { }

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
            struct { T u; T v; T w; };
        };

        struct MembersAxis { T x; T y; T z; };
        struct MembersColour { T r; T g; T b; };
        struct MembersEuler { T roll; T pitch; T yaw; };
        struct MembersUVW { T u; T v; T w; };

        constexpr Vec3() = default;

        constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) { }
        constexpr Vec3(T it) : Vec3(it, it, it){ }
        constexpr Vec3(const Vec2& xy, T z) : Vec3(xy.x, xy.y, z) { }
        constexpr Vec3(T x, const Vec2& yz) : Vec3(x, yz.x, yz.y) { }
        constexpr Vec3(const T *data) : Vec3(data[0], data[1], data[2]) { }

        constexpr Vec3(MembersAxis members) : Vec3(members.x, members.y, members.z) { }
        constexpr Vec3(MembersColour members) : Vec3(members.r, members.g, members.b) { }
        constexpr Vec3(MembersEuler members) : Vec3(members.roll, members.pitch, members.yaw) { }
        constexpr Vec3(MembersUVW members) : Vec3(members.u, members.v, members.w) { }

        template<typename O> requires (!std::is_same_v<T, O>)
        constexpr Vec3(Vec3<O> other) : Vec3((T)other.x, (T)other.y, (T)other.z) { }

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

        constexpr Vec2 xy() const { return Vec2(x, y); }
        constexpr Vec2 xz() const { return Vec2(x, z); }
        constexpr Vec2 yz() const { return Vec2(y, z); }

        bool isinf() const { return std::isinf(x) || std::isinf(y) || std::isinf(z); }

        constexpr bool is_uniform() const { return x == y && y == z; }

        constexpr Vec3 negate() const { return Vec3(-x, -y, -z); }
        constexpr Vec3 abs() const { return Vec3(std::abs(x), std::abs(y), std::abs(z)); }
        constexpr T length() const { return std::sqrt(x * x + y * y + z * z); }
        constexpr T length_squared() const { return x * x + y * y + z * z; }

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
    constexpr Vec3<T> operator*(T scalar, Vec3<T> vec) {
        return Vec3<T>(scalar * vec.x, scalar * vec.y, scalar * vec.z);
    }

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

        struct MembersAxis { T x; T y; T z; T w; };
        struct MembersColour { T r; T g; T b; T a; };

        constexpr Vec4() = default;

        constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
        constexpr Vec4(T it) : Vec4(it, it, it, it) { }
        constexpr Vec4(const Vec3& xyz, T w) : Vec4(xyz.x, xyz.y, xyz.z, w) { }
        constexpr Vec4(const T *data) : Vec4(data[0], data[1], data[2], data[3]) { }

        constexpr Vec4(MembersAxis members) : Vec4(members.x, members.y, members.z, members.w) { }
        constexpr Vec4(MembersColour members) : Vec4(members.r, members.g, members.b, members.a) { }

        template<typename O> requires (!std::is_same_v<T, O>)
        constexpr Vec4(Vec4<O> other) : Vec4((T)other.x, (T)other.y, (T)other.z, (T)other.w) { }

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

        constexpr Vec4 reciprocal() const {
            return Vec4(1 / x, 1 / y, 1 / z, 1 / w);
        }

        static constexpr Vec4 dot(const Vec4& lhs, const Vec4& rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
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

    // NOLINTBEGIN
    template<IsVector T>
    constexpr T fma(T a, T b, T c) {
        return c + (a * b);
    }

    enum Channel {
        X = 0,
        Y = 1,
        Z = 2,
        W = 3,
    };

    enum Side { A = 0, B = 0xFFFFFFFF };

    constexpr uint8 swizzle_mask(Channel x, Channel y, Channel z, Channel w) {
        return (x << 0) | (y << 2) | (z << 4) | (w << 6);
    }

    constexpr Channel swizzle_get(uint8 mask, Channel channel) {
        return static_cast<Channel>((mask >> (channel * 2)) & 0x3);
    }

    constexpr uint8 swizzle_set(uint8 mask, Channel channel, Channel value) {
        return (mask & ~(0x3 << (channel * 2))) | (value << (channel * 2));
    }

    template<IsVector T>
    constexpr T swizzle(T v, uint8 mask) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.fields[i] = v.fields[(mask >> (i * 2)) & 0x3];
        }
        return out;
    }

    template<IsVector T>
    constexpr T swizzle(T v, Channel x = X, Channel y = Y, Channel z = Z, Channel w = W) {
        return swizzle(v, swizzle_mask(x, y, z, w));
    }

    template<IsVector T>
    constexpr bool nearly_equal(T lhs, T rhs, typename T::Type epsilon) {
        return (lhs - rhs).length() < epsilon;
    }

    template<typename T>
    constexpr Vec4<T> select(Vec4<T> a, Vec4<T> b, Side x, Side y, Side z, Side w) {
        T sx = x == Side::A ? a.x : b.x;
        T sy = y == Side::A ? a.y : b.y;
        T sz = z == Side::A ? a.z : b.z;
        T sw = w == Side::A ? a.w : b.w;

        return Vec4<T>(sx, sy, sz, sw);
    }

    enum PermuteChannel {
        X0 = 0, Y0 = 1, Z0 = 2, W0 = 3,
        X1 = 4, Y1 = 5, Z1 = 6, W1 = 7,
    };

    template<typename T>
    constexpr Vec4<T> permute(Vec4<T> lhs, Vec4<T> rhs, PermuteChannel x, PermuteChannel y, PermuteChannel z, PermuteChannel w) {
        T data[8] = { lhs.x, lhs.y, lhs.z, lhs.w, rhs.x, rhs.y, rhs.z, rhs.w };
        T px = data[x];
        T py = data[y];
        T pz = data[z];
        T pw = data[w];

        return Vec4<T>(px, py, pz, pw);
    }

    // NOLINTEND
}