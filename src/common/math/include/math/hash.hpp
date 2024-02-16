#pragma once

#include "core/hash.hpp"

#include "math.hpp"

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
