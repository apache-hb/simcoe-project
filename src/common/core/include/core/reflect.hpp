#pragma once

#include "reflect/reflect.h"

#include "fmtlib/format.h"

template<ctu::Reflected T>
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
        return fmt::format_to(ctx.out(), "{}", ctu::reflect<T>().to_string(value, base).data());
    }
};

template<>
struct fmt::formatter<ctu::ObjectName> : formatter<std::string_view> {
    using super = formatter<std::string_view>;
    auto format(const ctu::ObjectName& value, fmt::format_context& ctx) const {
        return super::format(value.data(), ctx);
    }
};

template<size_t N>
struct fmt::formatter<ctu::SmallString<N>> : formatter<std::string_view> {
    using super = formatter<std::string_view>;
    auto format(const ctu::SmallString<N>& value, fmt::format_context& ctx) const {
        return super::format(value.data(), ctx);
    }
};
