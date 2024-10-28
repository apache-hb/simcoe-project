#pragma once

#include "render/base/object.hpp"

namespace sm::render::next {
    render::Blob compileShader(std::string_view shader);
}
