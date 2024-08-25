#pragma once

#ifdef __REFLECT__
#   define REFLECT(...) [simcoe_meta(__VA_ARGS__)]
#   define REFLECT_BODY(id)
#   define REFLECT_ENUM(id, ...) REFLECT(__VA_ARGS__)
#else
#   define REFLECT(...)
#   define REFLECT_BODY(id) REFLECT_IMPL_##id
#   define REFLECT_ENUM(id, ...) REFLECT_BODY(id)
#endif

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
    struct ReflectTag final : ReflectTagBase {
        T data;

        consteval ReflectTag() noexcept : ReflectTagBase(I), data() { }
        consteval ReflectTag(T value) noexcept : ReflectTag(), data(value) { }

        consteval ReflectTag operator=(T value) const noexcept {
            return ReflectTag{value};
        }

        consteval ReflectTag operator()(T value) const noexcept {
            return ReflectTag{value};
        }
    };

    using Annotate = ReflectTag<const char*, eAnnotate>;
    constexpr Annotate annotate{}; // NOLINT(readability-identifier-naming)

    using Name = ReflectTag<const char*, eName>;
    constexpr Name name{}; // NOLINT(readability-identifier-naming)
}
