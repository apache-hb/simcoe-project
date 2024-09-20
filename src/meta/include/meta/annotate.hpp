#pragma once

#include <span>
#include <typeinfo>
#include <string_view>

namespace sm::reflect {
    class Class;
    class EnumClass;
    class Member;
    class EnumMember;
    class Property;
    class Method;

    struct Annotation {
        const std::type_info& type;
        const void *value;
    };

    enum class Visibility {
        ePublic,
        eProtected,
        ePrivate
    };

    class Annotated {
        std::span<const Annotation> mAnnotations;

        const void *findAnnotation(const std::type_info& type) const noexcept;

    protected:
        constexpr Annotated(std::span<const Annotation> annotations) noexcept
            : mAnnotations(annotations)
        { }

    public:
        template<typename T>
        const T *findAnnotation() const noexcept {
            return static_cast<T*>(findAnnotation(typeid(T)));
        }

        template<typename T>
        const T& getAnnotation() const noexcept {
            CTASSERTF(hasAnnotation<T>(), "Annotation not found: %s", typeid(T).name());
            return *findAnnotation<T>();
        }

        template<typename T>
        bool hasAnnotation() const noexcept {
            return findAnnotation<T>() != nullptr;
        }
    };

    class Declaration : public Annotated {
        std::string_view mName;
        std::string_view mPackageName;

    protected:
        constexpr Declaration(std::span<const Annotation> annotations, std::string_view name, std::string_view package) noexcept
            : Annotated(annotations)
            , mName(name)
            , mPackageName(package)
        { }

    public:
        std::string_view getName() const noexcept { return mName; }
        std::string_view getPackageName() const noexcept { return mPackageName; }
    };

    class Class : public Declaration {
    public:
        const Class &getSuperClass() const noexcept;
        bool hasSuperClass() const noexcept;

        std::span<const Property> getProperties() const noexcept;
        std::span<const Method> getMethods() const noexcept;
    };

    class EnumMember : public Declaration {
    public:
        EnumClass& getOwningEnum() const noexcept;
    };

    class EnumClass : public Declaration {
        std::span<const EnumMember> mMembers;
    public:
        constexpr EnumClass(std::span<const Annotation> annotations, std::string_view name, std::string_view package, std::span<const EnumMember> members) noexcept
            : Declaration(annotations, name, package)
            , mMembers(members)
        { }

        std::span<const EnumMember> getMembers() const noexcept { return mMembers; }
    };

    class Member : public Declaration {
    public:
        const Class& getOwningClass() const noexcept;
        Visibility getVisibility() const noexcept;
    };

    class Property : public Member {
    public:
        const Class& getType() const noexcept;

        size_t getOffsetOf() const noexcept;
        size_t getAlignOf() const noexcept;
        size_t getSizeOf() const noexcept;
    };

    class Method : public Member {
    public:
        const Class& getReturnType() const noexcept;
        std::span<const Class> getParameterTypes() const noexcept;
    };

    namespace detail {
        template<typename T>
        struct ClassImpl;
    }
}
