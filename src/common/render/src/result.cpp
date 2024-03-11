#include "render/result.hpp"

#include "stdafx.hpp"

using namespace sm;
using namespace sm::render;

CT_NORETURN
sm::assert_hresult(source_info_t source, Result hr, std::string_view msg) {
    sm::vpanic(source, "hresult {}: {}", hr, msg);
}

char *sm::render::Result::to_string() const {
    return os_error_string(mValue, sm::global_arena());
}
