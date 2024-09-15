#pragma once

#include <simcoe_config.h>

#include "reflect/reflect.h"
#include "fmtlib/format.h"

namespace sm {
    using FormatBuffer = fmt::basic_memory_buffer<char, 256>;
}

#if SMC_DEBUG
template<ctu::Reflected T> requires (ctu::is_enum<T>())
struct fmt::formatter<T> {
    int base = 10;

    constexpr auto parse(format_parse_context& ctx) {
        auto it = ctx.begin(), end = ctx.end();
        if (it != end) {

            switch (*it) {
            case 'x': case 'X': base = 16; break;
            case 'd': case 'D': base = 10; break;
            case 'o': case 'O': base = 8; break;
            case 'b': case 'B': base = 2; break;
            default: return it;
            }

            ++it;
        }
        return it;
    }

    auto format(const T& value, fmt::format_context& ctx) const {
        using Reflect = ctu::TypeInfo<T>;
        return fmt::format_to(ctx.out(), "{}", Reflect::to_string(value, base).data());
    }
};
#else
template<ctu::Reflected T> requires (ctu::is_enum<T>())
struct fmt::formatter<T> : fmt::formatter<typename T::Underlying> {
    using Super = fmt::formatter<typename T::Underlying>;
    auto format(const T& value, fmt::format_context& ctx) const {
        return Super::format(value.as_integral(), ctx);
    }
};
#endif
