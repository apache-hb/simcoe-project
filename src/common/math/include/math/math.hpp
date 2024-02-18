#pragma once

#include <cmath>
#include <cstdint>

#include "base/panic.h"

namespace sm::math {
    template<typename T>
    constexpr inline T kPi = T(3.14159265358979323846264338327950288);

    template<typename T>
    constexpr inline T kRadToDeg = T(180) / kPi<T>;

    template<typename T>
    constexpr inline T kDegToRad = kPi<T> / T(180);

    template<typename T>
    constexpr T to_radians(T degrees) {
        return degrees * kDegToRad<T>;
    }

    template<typename T>
    constexpr T to_degrees(T radians) {
        return radians * kRadToDeg<T>;
    }

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

    template<typename T> struct Mat4x4;

    template<typename T>
    struct Vec2 {
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
        constexpr Vec3(const Vec2<T>& xy, T z) : Vec3(xy.x, xy.y, z) { }
        constexpr Vec3(T x, const Vec2<T>& yz) : Vec3(x, yz.x, yz.y) { }
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

        constexpr Vec2<T> xy() const { return Vec2<T>(x, y); }
        constexpr Vec2<T> xz() const { return Vec2<T>(x, z); }
        constexpr Vec2<T> yz() const { return Vec2<T>(y, z); }

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
    struct Vec4 {
        static constexpr size_t kSize = 4;
        using Type = T;

        union {
            T fields[4];
            struct { T x; T y; T z; T w; };
            struct { T r; T g; T b; T a; };
        };

        constexpr Vec4() = default;

        constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { }
        constexpr Vec4(T it) : Vec4(it, it, it, it) { }
        constexpr Vec4(const Vec3<T>& xyz, T w) : Vec4(xyz.x, xyz.y, xyz.z, w) { }
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

        constexpr Vec3<T> xyz() const { return Vec3<T>(x, y, z); }

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

#if 0
    template<typename T>
    struct Mat3x3 {
        using Type = T;
        using RowType = Vec3<T>;
        using Mat4x4Type = Mat4x4<T>;

        RowType rows[3];

        constexpr Mat3x3() = default;
        constexpr Mat3x3(T it) : Mat3x3(RowType(it)) { }
        constexpr Mat3x3(const RowType& row) : Mat3x3(row, row, row) { }
        constexpr Mat3x3(const RowType& row0, const RowType& row1, const RowType& row2) : rows{ row0, row1, row2 } { }

        constexpr Mat3x3(const Mat4x4Type& other) : Mat3x3(
            RowType(other.at(0).xyz()),
            RowType(other.at(1).xyz()),
            RowType(other.at(2).xyz())
        ) { }

        static constexpr Mat3x3 identity() {
            RowType row1 = { 1, 0, 0 };
            RowType row2 = { 0, 1, 0 };
            RowType row3 = { 0, 0, 1 };
            return { row1, row2, row3 };
        }

        constexpr RowType at(size_t it) const { return rows[it]; }
    };
#endif

    template<typename T>
    struct Mat4x4 {
        using Type = T;
        using Row = Vec4<T>;
        using Row3 = Vec3<T>;

        union {
            T fields[16];
            Row rows[4];
        };

        constexpr Mat4x4() = default;

        constexpr Mat4x4(T it) : Mat4x4(Row(it)) { }
        constexpr Mat4x4(const Row& row) : Mat4x4(row, row, row, row) { }
        constexpr Mat4x4(const Row& row0, const Row& row1, const Row& row2, const Row& row3) : rows{ row0, row1, row2, row3 } { }
        constexpr Mat4x4(const T *data) : Mat4x4(Row(data), Row(data + 4), Row(data + 8), Row(data + 12)) { }

        constexpr Row column(size_t column) const {
            return { at(column, 0), at(column, 1), at(column, 2), at(column, 3) };
        }

        constexpr Row row(size_t row) const { return at(row); }

        constexpr const Row& at(size_t it) const { return rows[it]; }
        constexpr Row& at(size_t it) { return rows[it]; }

        constexpr const Row& operator[](size_t row) const { return rows[row];}
        constexpr Row& operator[](size_t row) { return rows[row]; }

        constexpr const T &at(size_t it, size_t col) const { return at(it).at(col); }
        constexpr T &at(size_t it, size_t col) { return at(it).at(col); }

        constexpr T *data() { return fields; }
        constexpr const T *data() const { return fields; }

        constexpr Row mul(const Row& other) const {
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

            Row out0 = {
                (other0.x * row0.x) + (other1.x * row0.y) + (other2.x * row0.z) + (other3.x * row0.w),
                (other0.y * row0.x) + (other1.y * row0.y) + (other2.y * row0.z) + (other3.y * row0.w),
                (other0.z * row0.x) + (other1.z * row0.y) + (other2.z * row0.z) + (other3.z * row0.w),
                (other0.w * row0.x) + (other1.w * row0.y) + (other2.w * row0.z) + (other3.w * row0.w)
            };

            Row out1 = {
                (other0.x * row1.x) + (other1.x * row1.y) + (other2.x * row1.z) + (other3.x * row1.w),
                (other0.y * row1.x) + (other1.y * row1.y) + (other2.y * row1.z) + (other3.y * row1.w),
                (other0.z * row1.x) + (other1.z * row1.y) + (other2.z * row1.z) + (other3.z * row1.w),
                (other0.w * row1.x) + (other1.w * row1.y) + (other2.w * row1.z) + (other3.w * row1.w)
            };

            Row out2 = {
                (other0.x * row2.x) + (other1.x * row2.y) + (other2.x * row2.z) + (other3.x * row2.w),
                (other0.y * row2.x) + (other1.y * row2.y) + (other2.y * row2.z) + (other3.y * row2.w),
                (other0.z * row2.x) + (other1.z * row2.y) + (other2.z * row2.z) + (other3.z * row2.w),
                (other0.w * row2.x) + (other1.w * row2.y) + (other2.w * row2.z) + (other3.w * row2.w)
            };

            Row out3 = {
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

        static constexpr Mat4x4 scale(const Row3& scale) {
            return Mat4x4::scale(scale.x, scale.y, scale.z);
        }

        static constexpr Mat4x4 scale(T x, T y, T z) {
            Row row0 = { x, 0, 0, 0 };
            Row row1 = { 0, y, 0, 0 };
            Row row2 = { 0, 0, z, 0 };
            Row row3 = { 0, 0, 0, 1 };
            return { row0, row1, row2, row3 };
        }

        constexpr Row3 getScale() const {
            return { at(0, 0), at(1, 1), at(2, 2) };
        }

        constexpr void setScale(const Row3& scale) {
            at(0, 0) = scale.x;
            at(1, 1) = scale.y;
            at(2, 2) = scale.z;
        }

        ///
        /// translation related functions
        ///

        static constexpr Mat4x4 translation(const Row3& translation) {
            return Mat4x4::translation(translation.x, translation.y, translation.z);
        }

        static constexpr Mat4x4 translation(T x, T y, T z) {
            Row row0 = { 1, 0, 0, x };
            Row row1 = { 0, 1, 0, y };
            Row row2 = { 0, 0, 1, z };
            Row row3 = { 0, 0, 0, 1 };
            return { row0, row1, row2, row3 };
        }

        constexpr Row3 translation() const {
            return { at(0, 3), at(1, 3), at(2, 3) };
        }

        constexpr void set_translation(const Row3& translation) {
            at(0, 3) = translation.x;
            at(1, 3) = translation.y;
            at(2, 3) = translation.z;
        }

        // rotation related functions

        /// @brief creates a rotation matrix from a vector
        /// @note the vector is expected to be in radians
        /// @param rotation the rotation vector
        /// @return the rotation matrix
        static constexpr Mat4x4 rotation(const Row3& rotation) {
            const auto& [pitch, yaw, roll] = rotation;
            const T cp = std::cos(pitch);
            const T sp = std::sin(pitch);

            const T cy = std::cos(yaw);
            const T sy = std::sin(yaw);

            const T cr = std::cos(roll);
            const T sr = std::sin(roll);

            Row r0 = {
                cr * cy + sr * sp * sy,
                sr * cp,
                sr * sp * cy - cr * sy,
                0
            };

            Row r1 = {
                cr * sp * sy - sr * cy,
                cr * cp,
                sr * sy + cr * sp * cy,
                0
            };

            Row r2 = {
                cp * sy,
                -sp,
                cp * cy,
                0
            };

            Row r3 = { 0, 0, 0, 1 };

            return { r0, r1, r2, r3 };
        }

        // full transform
        static constexpr Mat4x4 transform(const Row3& translation, const Row3& rotation, const Row3& scale) {
            return Mat4x4::translation(translation) * Mat4x4::rotation(rotation) * Mat4x4::scale(scale);
        }

        constexpr Mat4x4 transpose() const {
            Row r0 = { rows[0].x, rows[1].x, rows[2].x, rows[3].x };
            Row r1 = { rows[0].y, rows[1].y, rows[2].y, rows[3].y };
            Row r2 = { rows[0].z, rows[1].z, rows[2].z, rows[3].z };
            Row r3 = { rows[0].w, rows[1].w, rows[2].w, rows[3].w };
            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 identity() {
            Row row0 = { 1, 0, 0, 0 };
            Row row1 = { 0, 1, 0, 0 };
            Row row2 = { 0, 0, 1, 0 };
            Row row3 = { 0, 0, 0, 1 };
            return { row0, row1, row2, row3 };
        }

        // camera related functions

        static constexpr Mat4x4 lookToLH(const Row3& eye, const Row3& dir, const Row3& up) {
            CTASSERT(eye != Row3::zero());
            CTASSERT(up != Row3::zero());

            CTASSERT(!eye.isinf());
            CTASSERT(!up.isinf());

            auto r2 = dir.normalized();
            auto r0 = Row3::cross(up, r2).normalized();
            auto r1 = Row3::cross(r2, r0);

            auto negEye = eye.negate();

            auto d0 = Row3::dot(r0, negEye);
            auto d1 = Row3::dot(r1, negEye);
            auto d2 = Row3::dot(r2, negEye);

            Row s0 = { r0, d0 };
            Row s1 = { r1, d1 };
            Row s2 = { r2, d2 };
            Row s3 = { 0, 0, 0, 1 };

            return Mat4x4(s0, s1, s2, s3).transpose();
        }

        static constexpr Mat4x4 lookToRH(const Row3& eye, const Row3& dir, const Row3& up) {
            return Mat4x4::lookToLH(eye, dir.negate(), up);
        }

        static constexpr Mat4x4 lookAtRH(const Row3& eye, const Row3& focus, const Row3& up) {
            return Mat4x4::lookToLH(eye, eye - focus, up);
        }

        static constexpr Mat4x4 perspectiveRH(T fov, T aspect, T nearLimit, T farLimit) {
            auto fovSin = std::sin(fov / 2);
            auto fovCos = std::cos(fov / 2);

            auto height = fovCos / fovSin;
            auto width = height / aspect;
            auto range = farLimit / (nearLimit - farLimit);

            Row r0 = { width, 0,      0,                 0 };
            Row r1 = { 0,     height, 0,                 0 };
            Row r2 = { 0,     0,      range,            -1 };
            Row r3 = { 0,     0,      range * nearLimit, 0 };
            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 orthographicRH(T width, T height, T nearLimit, T farLimit) {
            T range = 1 / (nearLimit - farLimit);

            Row r0 = { 2 / width, 0,          0,                 0 };
            Row r1 = { 0,         2 / height, 0,                 0 };
            Row r2 = { 0,         0,          range,             0 };
            Row r3 = { 0,         0,          range * nearLimit, 1 };

            return { r0, r1, r2, r3 };
        }

        static constexpr Mat4x4 orthographicRH(T left, T right, T top, T bottom, T nearLimit, T farLimit) {
            T range = 1 / (nearLimit - farLimit);

            T rWidth = 1.f / (right - left);
            T rHeight = 1.f / (top - bottom);

            Row r0 = { rWidth + rWidth, 0.f, 0.f, 0.f };
            Row r1 = { 0.f, rHeight + rHeight, 0.f, 0.f };
            Row r2 = { 0.f, 0.f, range, 0.f };

            Row r3 = {
                -(left + right) * rWidth,
                -(top + bottom) * rHeight,
                range * nearLimit,
                1.f
            };

            return { r0, r1, r2, r3 };
        }
    };

    template<typename T>
    concept IsVector = requires (T it) {
        { T::kSize } -> std::convertible_to<size_t>;
        { it.fields } -> std::convertible_to<typename T::Type*>;
    };

    template<IsVector T>
    constexpr T min(const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.at(i) = math::min(lhs.fields[i], rhs.fields[i]);
        }
        return out;
    }

    template<IsVector T>
    constexpr T max(const T& lhs, const T& rhs) {
        T out;
        for (size_t i = 0; i < T::kSize; i++) {
            out.at(i) = math::max(lhs.fields[i], rhs.fields[i]);
        }
        return out;
    }

    template<IsVector T>
    constexpr bool nearly_equal(const T& lhs, const T& rhs, typename T::Type epsilon) {
        return (lhs - rhs).length() < epsilon;
    }

    // NOLINTBEGIN

    using int2 = Vec2<int>;
    using int3 = Vec3<int>;
    using int4 = Vec4<int>;
    static_assert(sizeof(int2) == sizeof(int) * 2);
    static_assert(sizeof(int3) == sizeof(int) * 3);
    static_assert(sizeof(int4) == sizeof(int) * 4);

    using uint2 = Vec2<uint32_t>;
    using uint3 = Vec3<uint32_t>;
    using uint4 = Vec4<uint32_t>;
    static_assert(sizeof(uint2) == sizeof(uint32_t) * 2);
    static_assert(sizeof(uint3) == sizeof(uint32_t) * 3);
    static_assert(sizeof(uint4) == sizeof(uint32_t) * 4);

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

    using uint8x2 = Vec2<uint8_t>;
    using uint8x3 = Vec3<uint8_t>;
    using uint8x4 = Vec4<uint8_t>;
    static_assert(sizeof(uint8x2) == sizeof(uint8_t) * 2);
    static_assert(sizeof(uint8x3) == sizeof(uint8_t) * 3);
    static_assert(sizeof(uint8x4) == sizeof(uint8_t) * 4);

    using uint16x2 = Vec2<uint16_t>;
    using uint16x3 = Vec3<uint16_t>;
    using uint16x4 = Vec4<uint16_t>;
    static_assert(sizeof(uint16x2) == sizeof(uint16_t) * 2);
    static_assert(sizeof(uint16x3) == sizeof(uint16_t) * 3);
    static_assert(sizeof(uint16x4) == sizeof(uint16_t) * 4);

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
}

// NOLINTEND
