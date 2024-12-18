#pragma once

#include <typeinfo>

#define annotation struct

namespace sm::reflect {
    annotation Name {
        const char *value = nullptr;
    };

    annotation Description {
        const char *value = "No description provided.";
    };

    annotation Transient {
        bool value = true;
    };

    struct AnnotateBase {
        const std::type_info& type;
    };

    template<typename T>
    struct Annotate final : AnnotateBase {
        T value;

        consteval Annotate() noexcept
            : AnnotateBase(typeid(T))
            , value()
        { }

        consteval Annotate(auto value) noexcept
            : AnnotateBase(typeid(T))
            , value(T{value})
        { }

        consteval Annotate operator=(T value) const noexcept {
            return Annotate{value};
        }

        consteval Annotate operator()(T value) const noexcept {
            return Annotate{value};
        }
    };

    template<typename T>
    constexpr Annotate<T> annotate{}; // NOLINT(readability-identifier-naming)

    constexpr Annotate<sm::reflect::Name> name{}; // NOLINT(readability-identifier-naming)
    constexpr Annotate<sm::reflect::Description> desc{}; // NOLINT(readability-identifier-naming)
    constexpr Annotate<sm::reflect::Transient> transient{true}; // NOLINT(readability-identifier-naming)
}
