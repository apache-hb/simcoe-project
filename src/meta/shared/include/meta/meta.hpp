#pragma once

#include <exception>
#ifdef __REFLECT__
#   define REFLECT(...) [meta::reflect(__VA_ARGS__)]
#else
#   define REFLECT(...)
#endif

#define TAGS(...) #__VA_ARGS__

namespace sm::meta::detail {
    enum ReflectTagType {
        eNone = 0,
        eAnnotate = 1,
        eName = 2
    };

    struct ReflectTagBase {
        int type;
    };

    template<typename T, ReflectTagType I>
    struct ReflectTag : ReflectTagBase {
        T data;

        consteval ReflectTag() : ReflectTagBase(I), data() { }
        consteval ReflectTag(T value) : ReflectTag() , data(value) { }

        consteval ReflectTag operator=(T value) const noexcept {
            return ReflectTag(value);
        }

        consteval ReflectTag operator()(T value) const noexcept {
            return ReflectTag(value);
        }
    };

    using Annotate = ReflectTag<const char*, eAnnotate>;
    constexpr Annotate annotate{};

    using Name = ReflectTag<const char*, eName>;
    constexpr Name name{};

    std::exception_ptr ptr;
}
