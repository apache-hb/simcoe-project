#pragma once

#include <span>

namespace sm::reflect {
    class Method;
    class Field;
    class Class;

    template<typename T>
    concept Reflected = requires (T it) {
        noexcept(T::getClass());

        { T::getClass() } -> std::same_as<const Class&>;
    };

    class Class {
        const Class *mSuperClass;
        const std::string_view mName;
        const std::string_view mPackageName;

    protected:
        Class(const Class *superClass, std::string_view name, std::string_view package)
            : mSuperClass(superClass)
            , mName(name)
            , mPackageName(package)
        { }

    public:
        std::string_view getName() const noexcept { return mName; }
        std::string_view getPackageName() const noexcept { return mPackageName; }
        std::span<const Field> getFields() const noexcept;
        std::span<const Method> getMethods() const noexcept;

        bool hasSuperClass() const noexcept { return mSuperClass != nullptr; }
        const Class& getSuperClass() const noexcept { return *mSuperClass; }
    };

    namespace detail {
        template<typename T>
        class ClassImpl;
    }
}
