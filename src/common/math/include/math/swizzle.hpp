#pragma once

#include "math/utils.hpp"

namespace sm::math {
    /// @brief swizzle mixin for vector types
    /// @tparam T vector element type
    /// @tparam N vector size
    /// @tparam I swizzle indices
    template<typename T, size_t N, size_t... I>
    class SwizzleField {
        T mFields[N];

    public:
        using Result = Vec<T, sizeof...(I)>;

        constexpr operator Result() const requires (IsVector<T>) {
            Vec<uint, sizeof...(I)> mask = { I... };
            return gather(mask, mFields);
        }
    };
}

// TODO: use swizzle mixin in Vec2, Vec3, Vec4
