#pragma once

#include <initializer_list>

#define META_WRAPPER(name, type, args, ...) \
    constinit const auto name = [] {               \
        struct Wrapper {                      \
            consteval type operator=args const { \
                return type(__VA_ARGS__);                  \
            }                                  \
        };                                    \
        return Wrapper{};                     \
    }()

namespace sm::meta {
    namespace detail {
        constexpr unsigned kTagName = 1;
        constexpr unsigned kTagRange = 2;
        constexpr unsigned kTagCategory = 3;
        constexpr unsigned kTagBitFlags = 4;
        constexpr unsigned kTagTransient = 5;
        constexpr unsigned kTagThreadSafe = 6;
        constexpr unsigned kTagNotThreadSafe = 7;

        struct Tag {
            unsigned id;

            consteval Tag(unsigned id)
                : id(id)
            { }
        };

        struct NameTag : Tag {
            const char *name;

            consteval NameTag(const char *name)
                : Tag(kTagName)
                , name(name)
            { }
        };

        template<typename T>
        struct RangeTag : Tag {
            T min;
            T max;

            consteval RangeTag(T min, T max)
                : Tag(kTagRange)
                , min(min)
                , max(max)
            { }
        };

        struct CategoryTag : Tag {
            const char *category;

            consteval CategoryTag(const char *category)
                : Tag(kTagCategory)
                , category(category)
            { }
        };

        struct RangeWrapper {
            template<typename T>
            consteval auto operator=(std::initializer_list<T> value) const {
                return detail::RangeTag<T>{value.begin()[0], value.begin()[1]};
            }
        };
    }

    META_WRAPPER(name, detail::NameTag, (const char *name), name);
    META_WRAPPER(category, detail::CategoryTag, (const char *category), category);

    // NOLINTBEGIN
    constexpr const detail::Tag bitflags = detail::Tag(detail::kTagBitFlags);
    constexpr const detail::Tag transient = detail::Tag(detail::kTagTransient);

    constexpr auto range = [] {
        return detail::RangeWrapper{};
    }();

    constexpr auto threadsafe = [] {
        struct ThreadSafeTag : detail::Tag {
            consteval auto operator=(bool value) const {
                return detail::Tag{value ? detail::kTagThreadSafe : detail::kTagNotThreadSafe};
            }

            consteval ThreadSafeTag()
                : Tag(detail::kTagThreadSafe)
            { }
        };

        return ThreadSafeTag{};
    }();

    // NOLINTEND
}
