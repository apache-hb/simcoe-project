#include "stdafx.hpp"

#include "render/base/error.hpp"

namespace render = sm::render;

std::string render::errorString(HRESULT hr) {
    static constexpr size_t kBufferSize = 512;

    if (SUCCEEDED(hr))
        return "Success";

    char buffer[kBufferSize];
    size_t size = os_error_get_string(hr, buffer, sizeof(buffer));
    buffer[std::min(size, sizeof(buffer) - 1)] = '\0';
    return buffer;
}
