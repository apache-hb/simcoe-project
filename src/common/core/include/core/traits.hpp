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

    /// @brief check if a type is a specialization of a template
    /// @tparam Test type to check
    /// @tparam Ref template to check against
    /// @see https://stackoverflow.com/questions/16337610/how-to-know-if-a-type-is-a-specialization-of-stdvector
    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type {};

    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

    template<template<typename...> class C, typename T>
    concept IsSpecialization = is_specialization<T, C>::value;
}
