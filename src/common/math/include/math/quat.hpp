#pragma once

#include "math/units.hpp"
#include "math/vector.hpp"

namespace sm::math {
    template <typename T>
    struct alignas(sizeof(T) * 4) Quat {
        using Rad = Radians<T>;
        using Vec3 = Vec3<T>;
        using Rad3 = Radians<Vec3>;
        using Vec4 = Vec4<T>;

        Vec3 v;
        T w;

        constexpr Quat() = default;
        constexpr Quat(const Vec3 &v, T w)
            : v(v)
            , w(w)
        { }

        constexpr Quat(T x, T y, T z, T w)
            : v(x, y, z)
            , w(w)
        { }

        constexpr Quat(const T *data)
            : Quat(data[0], data[1], data[2], data[3])
        { }

        static constexpr Quat identity() {
            return {Vec3::zero(), 1};
        }

        constexpr Quat conjugate() const {
            return {-v, w};
        }

        constexpr static Quat fromEulerAngle(Rad3 euler) {
            auto [ey, ep, er] = euler.get_radians();
            auto [sr, cr] = math::sincos(er / 2);
            auto [sp, cp] = math::sincos(ep / 2);
            auto [sy, cy] = math::sincos(ey / 2);

            T x = cr * sp * cy + sr * cp * sy;
            T y = cr * cp * sy - sr * sp * cy;
            T z = sr * cp * cy - cr * sp * sy;
            T w = cr * cp * cy + sr * sp * sy;

            return Quat {x, y, z, w};
        }

        constexpr static Quat fromAxisAngle(const Vec3 &axis, Rad angle) {
            auto [s, c] = math::sincos(angle.get_radians() / 2);
            return Quat {axis.normalized() * s, c};
        }

        constexpr Rad3 asEulerAngle() const {
            T sinr_cosp = T(2) * (w * v.x + v.y * v.z);
            T cosr_cosp = T(1) - T(2) * (v.x * v.x + v.y * v.y);

            T sinp = std::sqrt(T(1) + T(2) * (w * v.y - v.x * v.z));
            T cosp = std::sqrt(T(1) - T(2) * (w * v.y - v.x * v.z));

            T siny_cosp = T(2) * (w * v.z + v.x * v.y);
            T cosy_cosp = T(1) - T(2) * (v.y * v.y + v.z * v.z);

            T roll = std::atan2(sinr_cosp, cosr_cosp);
            T pitch = T(2) * std::atan2(sinp, cosp) - math::kPi<T> / T(2);
            T yaw = std::atan2(siny_cosp, cosy_cosp);

            Vec3 euler = {roll, pitch, yaw};

            return Rad3{euler};
        }

        constexpr Quat rotated(const Quat& other) const {
            T t0 = (v.z   - v.y) * (other.v.y   - other.v.z);
            T t1 = (w + v.x) * (other.w + other.v.x);
            T t2 = (w - v.x) * (other.v.y   + other.v.z);
            T t3 = (v.y   + v.z) * (other.w - other.v.x);
            T t4 = (v.z   - v.x) * (other.v.x   - other.v.y);
            T t5 = (v.z   + v.x) * (other.v.x   + other.v.y);
            T t6 = (w + v.y) * (other.w - other.v.z);
            T t7 = (w - v.y) * (other.w + other.v.z);
            T t8 = t5 + t6 + t7;
            T t9 = T(0.5) * (t4 + t8);

            T x = t1 + t9 - t8;
            T y = t2 + t9 - t7;
            T z = t3 + t9 - t6;
            T w = t0 + t9 - t5;

            return Quat{x, y, z, w};
        }

        constexpr Quat operator*(const Quat& other) const {
            return rotated(other);
        }

        constexpr Quat operator*(T scalar) const {
            return {v * scalar, w * scalar};
        }

        constexpr Vec3 operator*(const Vec3& vec) const {
            return ((conjugate() * Quat(vec, T(0)) * *this).v);
        }

        constexpr Quat& operator*=(const Quat& other) {
            return *this = *this * other;
        }

        constexpr Quat& operator*=(T scalar) {
            return *this = *this * scalar;
        }

        constexpr Quat operator-() const {
            return {-v, -w};
        }

        constexpr void decompose(Vec3 &axis, T &real) const {
            axis = v.normalized();
            real = std::acos(w) * T(2);
        }
    };
}
