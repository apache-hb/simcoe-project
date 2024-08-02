#include "stdafx.hpp"

#include "render/result.hpp"

using namespace sm;
using namespace sm::render;

CT_NORETURN
sm::assert_hresult(source_info_t source, Result hr, std::string_view msg) {
    sm::vpanic(source, "hresult {}: {}", hr, msg);
}
