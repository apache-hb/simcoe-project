#pragma once

#include "math/quat.hpp"
#include "math/units.hpp"
#include "math/vector.hpp"

#include <DirectXMath.h>

namespace sm::math {
    template<typename T>
    struct alignas(sizeof(T) * 16) Mat4x4 {
        using Type = T;
        using Rad = Radians<T>;
        using Vec4 = Vec4<T>;
        using Vec3 = Vec3<T>;
        using Rad3 = Radians<Vec3>;
        using Quat = Quat<T>;

        static constexpr size_t kRowCount = 4;
        static constexpr size_t kColumnCount = 4;
        static constexpr size_t kSize = kRowCount * kColumnCount;

        union {
            T fields[16];
            T matrix[4][4];
            Vec4 rows[4];
        };

        constexpr Mat4x4() = default;

        constexpr Mat4x4(T it)
            : Mat4x4(Vec4(it))
        { }

        constexpr Mat4x4(const Vec4& row)
            : Mat4x4(row, row, row, row)
        { }

        constexpr Mat4x4(const Vec4& row0, const Vec4& row1, const Vec4& row2, const Vec4& row3)
            : rows{ row0, row1, row2, row3 }
        { }

        constexpr Mat4x4(const T *data)
            : Mat4x4(Vec4(data), Vec4(data + 4), Vec4(data + 8), Vec4(data + 12))
        { }

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

        constexpr const T &operator[](size_t row, size_t col) const { return at(row, col); }
        constexpr T &operator[](size_t row, size_t col) { return at(row, col); }

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

        constexpr Mat4x4 operator*(T other) const {
            return { at(0) * other, at(1) * other, at(2) * other, at(3) * other };
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
        /// @param rotation the rotation vector
        /// @return the rotation matrix
        static constexpr Mat4x4 rotation(const Rad3& rotation) {
            auto v = rotation.get_radians();
            auto [sr, cr] = math::sincos(v.roll);
            auto [sp, cp] = math::sincos(v.pitch);
            auto [sy, cy] = math::sincos(v.yaw);

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
            T w = q.w;

            Vec4 r0 = { w, z, -y, x };
            Vec4 r1 = { -z, w, x, y };
            Vec4 r2 = { y, -x, w, z };
            Vec4 r3 = { -x, -y, -z, w };

            Vec4 m0 = { w, z, -y, -x };
            Vec4 m1 = { -z, w, x, -y };
            Vec4 m2 = { y, -x, w, -z };
            Vec4 m3 = { x, y, z, w };

            Mat4x4 r = { r0, r1, r2, r3 };
            Mat4x4 m = { m0, m1, m2, m3 };

            return r * m;
        }

        // full transform
        static constexpr Mat4x4 transform(const Vec3& translation, const Rad3& rotation, const Vec3& scale) {
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

        constexpr Mat4x4 inverse() const {
            Mat4x4 mt = transpose();

            Vec4 v0[4];
            Vec4 v1[4];

            v0[0] = swizzle(mt.row(2), X, X, Y, Y);
            v1[0] = swizzle(mt.row(3), Z, W, Z, W);
            v0[1] = swizzle(mt.row(0), X, X, Y, Y);
            v1[1] = swizzle(mt.row(1), Z, W, Z, W);
            v0[2] = permute(mt.row(2), mt.row(0), X0, Z0, X1, Z1);
            v1[2] = permute(mt.row(3), mt.row(1), Y0, W0, Y1, W1);

            Vec4 d0 = v0[0] * v1[0];
            Vec4 d1 = v0[1] * v1[1];
            Vec4 d2 = v0[2] * v1[2];

            v0[0] = swizzle(mt.row(2), Z, W, Z, W);
            v1[0] = swizzle(mt.row(3), X, X, Y, Y);
            v0[1] = swizzle(mt.row(0), Z, W, Z, W);
            v1[1] = swizzle(mt.row(1), X, X, Y, Y);
            v0[2] = permute(mt.row(2), mt.row(0), Y0, W0, Y1, W1);
            v1[2] = permute(mt.row(3), mt.row(1), X0, Z0, X1, Z1);

            d0 -= v0[0] * v1[0];
            d1 -= v0[1] * v1[1];
            d2 -= v0[2] * v1[2];

            v0[0] = swizzle(mt.row(1), Y, Z, X, Y);
            v1[0] = permute(d0, d2, Y1, Y0, W0, X0);
            v0[1] = swizzle(mt.row(0), Z, X, Y, X);
            v1[1] = permute(d0, d2, W0, Y1, Y0, Z0);
            v0[2] = swizzle(mt.row(3), Y, Z, X, Y);
            v1[2] = permute(d1, d2, W1, Y0, W0, X0);
            v0[3] = swizzle(mt.row(2), Z, X, Y, X);
            v1[3] = permute(d1, d2, W0, W1, Y0, Z0);

            Vec4 c0 = v0[0] * v1[0];
            Vec4 c2 = v0[1] * v1[1];
            Vec4 c4 = v0[2] * v1[2];
            Vec4 c6 = v0[3] * v1[3];

            v0[0] = swizzle(mt.row(1), Z, W, Y, Z);
            v1[0] = permute(d0, d2, W0, X0, Y0, X1);
            v0[1] = swizzle(mt.row(0), W, Z, W, Y);
            v1[1] = permute(d0, d2, Z0, Y0, X1, X0);
            v0[2] = swizzle(mt.row(3), Z, W, Y, Z);
            v1[2] = permute(d1, d2, W0, X0, Y0, Z1);
            v0[3] = swizzle(mt.row(2), W, Z, W, Y);
            v1[3] = permute(d1, d2, Z0, Y0, Z1, X0);

            c0 -= v0[0] * v1[0];
            c2 -= v0[1] * v1[1];
            c4 -= v0[2] * v1[2];
            c6 -= v0[3] * v1[3];

            v0[0] = swizzle(mt.row(1), W, X, W, X);
            v1[0] = permute(d0, d2, Z0, Y1, X1, Z0);
            v0[1] = swizzle(mt.row(0), Y, W, X, Z);
            v1[1] = permute(d0, d2, Y1, X0, W0, X1);
            v0[2] = swizzle(mt.row(3), W, X, W, X);
            v1[2] = permute(d1, d2, Z0, W1, Z1, Z0);
            v0[3] = swizzle(mt.row(2), Y, W, X, Z);
            v1[3] = permute(d1, d2, W1, X0, W0, Z1);

            Vec4 c1 = c0 - (v0[0] * v1[0]);
            c0 += v0[0] * v1[0];

            Vec4 c3 = c2 + (v0[1] * v1[1]);
            c2 -= v0[1] * v1[1];

            Vec4 c5 = c4 + (v0[2] * v1[2]);
            c4 += v0[2] * v1[2];

            Vec4 c7 = c6 + (v0[3] * v1[3]);
            c6 -= v0[3] * v1[3];

            Mat4x4 r = {
                select(c0, c1, A, B, A, B),
                select(c2, c3, A, B, A, B),
                select(c4, c5, A, B, A, B),
                select(c6, c7, A, B, A, B)
            };

            Vec4 det = Vec4::dot(r.row(0), mt.row(0));

            Vec4 reciprocal = det.reciprocal();

            Mat4x4 result = {
                r.row(0) * reciprocal,
                r.row(1) * reciprocal,
                r.row(2) * reciprocal,
                r.row(3) * reciprocal
            };

            return result;
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
}
