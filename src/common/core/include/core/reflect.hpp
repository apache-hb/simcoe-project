#pragma once

#include "reflect/reflect.h"

#include <array>
#include <cstdio>
#include <type_traits>

namespace sm {
    template<typename T>
    concept IsEnum = std::is_enum_v<T>;

    template<typename T>
    consteval const char *get_printf_specifier();

#define PRINTF_SPEC(T, S) template<> consteval const char *get_printf_specifier<T>() { return S; }

    PRINTF_SPEC(bool, "%d")
    PRINTF_SPEC(char, "%c")
    PRINTF_SPEC(int32_t, "%d")
    PRINTF_SPEC(uint32_t, "%u")

    template<size_t... N>
    constexpr auto join(const char (&...strings)[N]) {
        constexpr size_t count = (... + N) - sizeof...(N);

        std::array<char, count + 1> result{};
        result[count] = '\0';

        char *ptr = result.data();
        ((ptr = std::copy(strings, strings + N - 1, ptr)), ...);

        return result;
    }

    template<size_t N = 32, IsEnum T>
    std::array<char, N> enum_to_string(T value) {
        constexpr auto refl = ctu::reflect<T>();
        using reflect_t = decltype(refl);
        std::array<char, N> str{};

        for (auto field : refl.get_cases()) {
            if (field.get_value() == value) {
                const char *id = field.get_name();
                std::copy(id, id + std::strlen(id), str.data());
                return str;
            }
        }

        auto fmt = join("<%s: ", get_printf_specifier<typename reflect_t::underlying_t>(), ">");

        std::snprintf(str, N, fmt.data(), refl.get_name(), refl.to_underlying(value));
        return str;
    }
}
