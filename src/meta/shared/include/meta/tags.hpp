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

#define META_BOOL_WRAPPER(name, tag, truthy, falsy, initial) \
    constexpr auto name = [] {                                \
        struct Wrapper : detail::Tag {                                      \
            consteval auto operator=(bool value) const {     \
                return detail::Tag{value ? truthy : falsy};                            \
            }                                                 \
                                                             \
            consteval Wrapper() \
                : detail::Tag(initial) \
            { }                                                 \
        };                                                   \
        return Wrapper{};                                    \
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
        constexpr unsigned kTagTypeId = 8;
        constexpr unsigned kTagMinimal = 9;
        constexpr unsigned kTagNotTransient = 10;
        constexpr unsigned kTagNotMinimal = 11;
        constexpr unsigned kTagNotBitFlags = 12;

        struct Tag {
            unsigned tag;

            consteval Tag(unsigned id)
                : tag(id)
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

        struct TypeIdTag : Tag {
            unsigned id;

            consteval TypeIdTag(unsigned id)
                : Tag(kTagTypeId)
                , id(id)
            { }
        };

        struct RangeWrapper {
            template<typename T>
            consteval auto operator=(std::initializer_list<T> value) const {
                return detail::RangeTag<T>{value.begin()[0], value.begin()[1]};
            }
        };
    }

    namespace tags {
        META_WRAPPER(name, detail::NameTag, (const char *name), name);
        META_WRAPPER(category, detail::CategoryTag, (const char *category), category);
        META_WRAPPER(id, detail::TypeIdTag, (unsigned id), id);

        // NOLINTBEGIN
        constexpr auto range = [] {
            return detail::RangeWrapper{};
        }();

        META_BOOL_WRAPPER(threadsafe, detail::kTagThreadSafe, detail::kTagThreadSafe, detail::kTagNotThreadSafe, detail::kTagThreadSafe);
        META_BOOL_WRAPPER(bitflags, detail::kTagBitFlags, detail::kTagBitFlags, detail::kTagNotBitFlags, detail::kTagBitFlags);
        META_BOOL_WRAPPER(transient, detail::kTagTransient, detail::kTagTransient, detail::kTagNotTransient, detail::kTagTransient);
        META_BOOL_WRAPPER(minimal, detail::kTagMinimal, detail::kTagMinimal, detail::kTagNotMinimal, detail::kTagMinimal);
        // NOLINTEND
    }
}
