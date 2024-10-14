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

render::RenderError::RenderError(HRESULT value)
    : Super(errorString(value), FAILED(value))
    , mValue(value)
{ }

render::RenderError::RenderError(HRESULT value, std::string_view expr)
    : Super(fmt::format("{}: {}", expr, errorString(value)), FAILED(value))
    , mValue(value)
{ }

render::RenderException::RenderException(HRESULT value, std::string_view expr)
    : Super(RenderError(value, expr))
{ }
