#pragma once

#include "core/arena.hpp"
#include "fmtlib/format.h"

namespace sm {
    using FormatBuffer = fmt::basic_memory_buffer<char, 256, sm::StandardArena<char>>;

    template<Pool::Inner P>
    using FormatPoolBuffer = fmt::basic_memory_buffer<char, 256, sm::StandardPool<char, P>>;
}
