#pragma once

#include "reflect/reflect.h"

#include "fmtlib/format.h"

template<ctu::Reflected T>
struct fmt::formatter<T> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const T& value, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", ctu::reflect<T>().to_string(value).data());
    }
};

template<>
struct fmt::formatter<ctu::ObjectName> : formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const ctu::ObjectName& value, FormatContext& ctx) const {
        return formatter<std::string_view>::format(value.data(), ctx);
    }
};

template<size_t N>
struct fmt::formatter<ctu::SmallString<N>> : formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const ctu::SmallString<N>& value, FormatContext& ctx) const {
        return formatter<std::string_view>::format(value.data(), ctx);
    }
};
