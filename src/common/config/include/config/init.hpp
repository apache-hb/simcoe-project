#pragma once

#include <string_view>

namespace sm::config {
    class Group;

    struct Name { std::string_view value; };
    struct Description { std::string_view value; };
    struct ReadOnly { bool readonly = true; };
    struct Hidden { bool hidden = true; };
    struct Category { Group& group; };

    template<typename T>
    struct EnumValue {
        std::string_view name;
        T value;
    };

    template<typename T>
    struct EnumValueBuilder {
        T value;

        constexpr EnumValue<T> operator=(std::string_view name) const {
            return EnumValue<T>{ name, value };
        }
    };

    template<typename T>
    using OptionList = std::initializer_list<EnumValue<T>>;

    template<typename T>
    struct EnumOptions {
        OptionList<T> options;
    };

    template<typename T>
    struct EnumFlags {
        OptionList<T> options;
    };

    template<typename T>
    struct InitialValue { T value; };

    namespace detail {
        template<typename T>
        constexpr inline T kMinRange = std::numeric_limits<T>::min();

        template<std::floating_point T>
        constexpr inline T kMinRange<T> = -std::numeric_limits<T>::max();

        template<typename T>
        constexpr inline T kMaxRange = std::numeric_limits<T>::max();
    }

    template<typename T>
    struct Range {
        T min = detail::kMinRange<T>;
        T max = detail::kMaxRange<T>;

        constexpr bool contains(T value) const noexcept {
            return value >= min && value <= max;
        }
    };

    struct ChoiceWrapper {
        template<typename T>
        constexpr auto operator=(std::initializer_list<EnumValue<T>> values) const {
            return EnumOptions<T>{values};
        }

        template<typename T>
        constexpr auto operator()(std::initializer_list<EnumValue<T>> values) const {
            return EnumOptions<T>{values};
        }
    };

    struct FlagsWrapper {
        template<typename T>
        constexpr auto operator=(std::initializer_list<EnumValue<T>> values) const {
            return EnumFlags<T>{values};
        }

        template<typename T>
        constexpr auto operator()(std::initializer_list<EnumValue<T>> values) const {
            return EnumFlags<T>{values};
        }
    };

    template<template<typename> typename T>
    struct ValueWrapper {
        template<typename V>
        constexpr T<V> operator=(T<V> value) const {
            return value;
        }

        template<typename V>
        constexpr T<V> operator()(T<V> value) const {
            return value;
        }

        template<typename V>
        constexpr T<V> operator=(V value) const {
            return T<V>{value};
        }

        template<typename V>
        constexpr T<V> operator()(V value) const {
            return T<V>{value};
        }
    };

    template<typename T, typename V>
    struct ConfigWrapper {
        constexpr T operator=(V value) const {
            return T{value};
        }

        constexpr T operator()(V value) const {
            return T{value};
        }
    };
}
