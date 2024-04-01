#pragma once

#include <span>
#include <string_view>

namespace sm::meta {
    class Class;
    class Method;
    class Property;

    class Method {

    };

    class Property {

    };

    class Class {
        std::string_view mName;
        std::string_view mQualifiedName;
        std::string_view mCategory;

        std::span<const Class* const> mBaseClasses;
        std::span<const Method> mMethods;
        std::span<const Property> mProperties;

    public:
        consteval Class(
            std::string_view name,
            std::string_view qual,
            std::string_view category,
            std::span<const Class* const> bases,
            std::span<const Method> methods,
            std::span<const Property> properties)
            : mName(name)
            , mQualifiedName(qual)
            , mCategory(category)
            , mBaseClasses(bases)
            , mMethods(methods)
            , mProperties(properties)
        { }

        constexpr std::string_view name() const { return mName; }
        constexpr std::string_view qualified() const { return mQualifiedName; }
        constexpr std::string_view category() const { return mCategory; }
        constexpr std::span<const Class* const> bases() const { return mBaseClasses; }
        constexpr std::span<const Method> methods() const { return mMethods; }
        constexpr std::span<const Property> properties() const { return mProperties; }
    };

    namespace detail {
        template<typename T>
        struct GlobalClass { };
    }
}
