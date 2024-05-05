#pragma once

#include <type_traits>

namespace sm {
    template<typename T>
    concept StandardLayout = std::is_standard_layout_v<T>;

    template<typename T>
    concept IsEnum = std::is_enum_v<T>;

    template<typename T>
    concept IsArray = std::is_array_v<T>;

    template<typename T, typename... A>
    concept Construct = std::is_constructible_v<T, A...>;
}
