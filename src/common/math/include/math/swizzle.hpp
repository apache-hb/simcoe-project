#pragma once

#include "math/utils.hpp"

namespace sm::math {
    template<IsVector T, size_t... I>
    class SwizzleField {
        T mBody;

    public:
        constexpr operator T() const {
            Vec<uint, T::kSize> mask = { I... };
            return gather(mask, mBody);
        }
    };
}
