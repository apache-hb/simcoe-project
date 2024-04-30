#pragma once

#include "math/vector.hpp"

namespace sm::math {
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
}
