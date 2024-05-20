#pragma once

#include <span>
#include <array>
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

        consteval EnumValue<T> operator=(std::string_view name) const {
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

    template<typename T>
    struct Range { T min; T max; };

    consteval auto val(auto value) {
        using T = decltype(value);
        using Builder = EnumValueBuilder<T>;

        return Builder{value};
    }

    struct OptionWrapper {
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
        consteval T<V> operator=(V value) const {
            return T<V>{value};
        }

        template<typename V>
        consteval T<V> operator()(V value) const {
            return T<V>{value};
        }
    };

    template<typename T, typename V>
    struct ConfigWrapper {
        consteval T operator=(V value) const {
            return T{value};
        }

        consteval T operator()(V value) const {
            return T{value};
        }
    };
}
