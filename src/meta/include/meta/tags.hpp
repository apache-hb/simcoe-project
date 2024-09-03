#pragma once

namespace sm::reflect::detail {
    enum ReflectTagType {
        eNone = 0,
        eAnnotate = 1,
        eName = 2,
        eDescription = 3,
        eTransient = 4,
    };

    struct ReflectTagBase {
        int type;
    };

    template<typename T, ReflectTagType I>
    struct ReflectTag final : ReflectTagBase {
        T data;

        consteval ReflectTag() noexcept : ReflectTagBase(I), data() { }
        consteval ReflectTag(T value) noexcept : ReflectTagBase(I), data(value) { }

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

    using Description = ReflectTag<const char*, eDescription>;
    constexpr Description desc{}; // NOLINT(readability-identifier-naming)

    using Transient = ReflectTag<bool, eTransient>;
    constexpr Transient transient{true}; // NOLINT(readability-identifier-naming)
}
