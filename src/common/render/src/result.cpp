#include "render/result.hpp"

#include "core/arena.hpp"
#include "core/error.hpp"

#include "os/core.h"

using namespace sm;
using namespace sm::render;

CT_NORETURN
sm::assert_hresult(source_info_t source, const char *expr, Result hr) {
    sm::vpanic(source, "hresult {}: {}", hr, expr);
}

char *sm::render::Result::to_string() const {
    return os_error_string(mValue, sm::global_arena());
}
