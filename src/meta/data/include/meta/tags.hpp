#pragma once

namespace sm::meta {
    namespace detail {
        constexpr unsigned kTagName = 1;
        constexpr unsigned kTagRange = 2;
        constexpr unsigned kTagCategory = 3;

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

        struct Range {
            int min;
            int max;

            consteval Range(int min, int max)
                : min(min)
                , max(max)
            { }
        };

        struct RangeTag : Tag {
            Range range;

            consteval RangeTag(int min, int max)
                : Tag(kTagRange)
                , range(min, max)
            { }
        };

        struct CategoryTag : Tag {
            const char *category;

            consteval CategoryTag(const char *category)
                : Tag(kTagCategory)
                , category(category)
            { }
        };
    }

    constinit auto name = [] {
        struct Wrapper {
            consteval auto operator=(const char *name) const {
                return detail::NameTag(name);
            }
        };

        return Wrapper{};
    }();

    constinit auto range = [] {
        struct Wrapper {
            consteval auto operator=(const detail::Range &range) const {
                return detail::RangeTag(range.min, range.max);
            }
        };

        return Wrapper{};
    }();

    constinit auto category = [] {
        struct Wrapper {
            consteval auto operator=(const char *category) const {
                return detail::CategoryTag(category);
            }
        };

        return Wrapper{};
    }();
}
