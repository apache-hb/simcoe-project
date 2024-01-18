#include "core/text.hpp"

#include "std/str.h"

using namespace sm;

Text Text::format(arena_t *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Text result = vformat(arena, fmt, args);
    va_end(args);
    return result;
}

Text Text::vformat(arena_t *arena, const char *fmt, va_list args) {
    return text_vformat(arena, fmt, args);
}
