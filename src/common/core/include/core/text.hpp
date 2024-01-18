#pragma once

#include <sm_core_api.hpp>

#include <cstdarg>
#include <string_view>

#include "core/text.h"

typedef struct arena_t arena_t;

namespace sm {
    class SM_CORE_API Text : private text_t {

    public:
        static Text format(arena_t *arena, const char *fmt, ...);
        static Text vformat(arena_t *arena, const char *fmt, va_list args);

        constexpr const char *data() const { return text; }
        constexpr size_t count() const { return length; }
        constexpr std::string_view view() const { return { text, text + length }; }

        constexpr const char *begin() const { return text; }
        constexpr const char *end() const { return text + length; }

        constexpr Text(text_t text)
            : text_t(text)
        { }
    };
}