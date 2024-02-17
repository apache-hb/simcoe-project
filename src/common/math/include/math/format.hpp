#pragma once

#include "math.hpp"

#include "fmtlib/format.h"

template<typename T>
struct fmt::formatter<sm::math::Vec2<T>> : fmt::nested_formatter<T> {
    using super = fmt::nested_formatter<T>;

    auto format(const sm::math::Vec2<T>& value, fmt::format_context& ctx) const {
        return super::write_padded(ctx, [this, value](auto out) {
            return fmt::format_to(out, "({}, {})", this->nested(value.x), this->nested(value.y));
        });
    }
};

template<typename T>
struct fmt::formatter<sm::math::Vec3<T>> : fmt::nested_formatter<T> {
    using super = fmt::nested_formatter<T>;

    auto format(const sm::math::Vec3<T>& value, fmt::format_context& ctx) const {
        return super::write_padded(ctx, [this, value](auto out) {
            return fmt::format_to(out, "({}, {}, {})", this->nested(value.x), this->nested(value.y), this->nested(value.z));
        });
    }
};

template<typename T>
struct fmt::formatter<sm::math::Vec4<T>> : fmt::nested_formatter<T> {
    using super = fmt::nested_formatter<T>;

    auto format(const sm::math::Vec4<T>& value, fmt::format_context& ctx) const {
        return super::write_padded(ctx, [this, value](auto out) {
            return fmt::format_to(out, "({}, {}, {}, {})", this->nested(value.x), this->nested(value.y), this->nested(value.z), this->nested(value.w));
        });
    }
};
