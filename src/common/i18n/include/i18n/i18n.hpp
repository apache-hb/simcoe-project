#pragma once

#include "core/map.hpp"
#include "core/text.hpp"
#include "core/utf8.hpp"

#include "fmtlib/format.h"

#include "i18n.reflect.h"

namespace sm::i18n {
    struct Plural {
        std::string_view text;
        bool plural;
    };

    class Table {
        Language m_lang;
        sm::Map<sm::String, utf8::StaticText> m_strings;

    public:
        Table(Language lang) : m_lang(lang) { }

        void add_string(std::string_view key, utf8::StaticText value) {
            m_strings.emplace(key, value);
        }

        utf8::StaticText get_string(const sm::String& key) const {
            if (auto it = m_strings.find(key); it != m_strings.end()) {
                return it->second;
            }
            return u8"<unknown>";
        }
    };

    constexpr Plural plural(std::string_view text, bool plural) {
        return { text, plural };
    }
}

template<>
struct fmt::formatter<sm::i18n::Plural> : fmt::formatter<std::string_view> {
    using super = fmt::formatter<std::string_view>;

    auto format(const sm::i18n::Plural& plural, fmt::format_context& ctx) const {
        return fmt::format_to(ctx.out(), "{}{}", plural.text, plural.plural ? "s" : "");
    }
};
