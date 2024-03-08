#pragma once

#include "math/math.hpp"

#include "fmtlib/format.h"

template<sm::math::IsAngle T> requires (sm::math::IsVector<typename T::Type>)
struct fmt::formatter<T> : fmt::nested_formatter<typename T::Type::Type> {
    using Vec = typename T::Type;
    using Super = fmt::nested_formatter<typename T::Type::Type>;

    const char *mode = (std::is_same_v<T, sm::math::Degrees<Vec>>) ? "deg" : "rad";

    constexpr auto format(const T& angle, fmt::format_context& ctx) const {
        return Super::write_padded(ctx, [this, angle](auto out) {
            out = fmt::format_to(out, "(");
            for (size_t i = 0; i < Vec::kSize; i++) {
                if (i > 0) {
                    out = fmt::format_to(out, ", ");
                }
                out = fmt::format_to(out, "{}{}", this->nested(angle.value[i]), mode);
            }
            return fmt::format_to(out, ")");
        });
    }
};

template<sm::math::IsAngle T>
struct fmt::formatter<T> : fmt::formatter<typename T::Type> {
    using Super = fmt::formatter<typename T::Type>;

    const char *mode = (std::is_same_v<T, sm::math::Degrees<typename T::Type>>) ? "deg" : "rad";

    constexpr auto format(const T& angle, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}{}", angle.value, mode);
    }
};

template<sm::math::IsVector T>
struct fmt::formatter<T> : fmt::nested_formatter<typename T::Type> {
    using Super = fmt::nested_formatter<typename T::Type>;

    constexpr auto format(const T& value, fmt::format_context& ctx) const {
        return Super::write_padded(ctx, [this, value](auto out) {
            out = fmt::format_to(out, "(");
            for (size_t i = 0; i < T::kSize; i++) {
                if (i > 0) {
                    out = fmt::format_to(out, ", ");
                }
                out = fmt::format_to(out, "{}", this->nested(value.fields[i]));
            }
            return fmt::format_to(out, ")");
        });
    }
};
