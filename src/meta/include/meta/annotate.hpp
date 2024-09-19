#pragma once

#include <typeinfo>
#include <string_view>

namespace sm::reflect {
    class Annotated {
    public:
        const void *getAnnotation(const std::type_info& type) const noexcept {
            return nullptr;
        }

        template<typename T>
        const T *getAnnotation() const noexcept {
            return static_cast<T*>(getAnnotation(typeid(T)));
        }

        template<typename T>
        bool hasAnnotation() const noexcept {
            return getAnnotation<T>() != nullptr;
        }
    };

    class Class : public Annotated {

    };

    class Member : public Annotated {
    public:
        Class& getOwningClass() const noexcept;
        std::string_view getName() const noexcept;
    };

    class Property : public Member {

    };

    class Method : public Member {

    };
}
