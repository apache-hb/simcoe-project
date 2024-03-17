#pragma once

#include "core/hash.hpp"

#include "math.hpp"

// NOLINTBEGIN(cert-dcl58-cpp)
template<typename T>
struct std::hash<sm::math::Vec2<T>> {
    size_t operator()(const sm::math::Vec2<T>& v) const {
        size_t seed = 0;
        sm::hash_combine(seed, v.x, v.y);
        return seed;
    }
};

template<typename T>
struct std::hash<sm::math::Vec3<T>> {
    size_t operator()(const sm::math::Vec3<T>& v) const {
        size_t seed = 0;
        sm::hash_combine(seed, v.x, v.y, v.z);
        return seed;
    }
};

template<typename T>
struct std::hash<sm::math::Vec4<T>> {
    size_t operator()(const sm::math::Vec4<T>& v) const {
        size_t seed = 0;
        sm::hash_combine(seed, v.x, v.y, v.z, v.w);
        return seed;
    }
};

template<typename T>
struct std::hash<sm::math::Quat<T>> {
    size_t operator()(const sm::math::Quat<T>& q) const {
        size_t seed = 0;
        sm::hash_combine(seed, q.x, q.y, q.z, q.w);
        return seed;
    }
};

// use get_degrees() in both as hash(180deg) == hash(2pi rad) should be true

template<typename T>
struct std::hash<sm::math::Degrees<T>> {
    size_t operator()(const sm::math::Degrees<T>& m) const {
        size_t seed = 0;
        sm::hash_combine(seed, m.get_degrees());
        return seed;
    }
};

template<typename T>
struct std::hash<sm::math::Radians<T>> {
    size_t operator()(const sm::math::Radians<T>& m) const {
        size_t seed = 0;
        sm::hash_combine(seed, m.get_degrees());
        return seed;
    }
};
// NOLINTEND(cert-dcl58-cpp)
