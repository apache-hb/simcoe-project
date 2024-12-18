#include "render/next/viewport.hpp"

namespace rn = sm::render::next;

rn::Viewport rn::Viewport::letterbox(math::uint2 display, math::uint2 render) {
    auto [renderWidth, renderHeight] = math::float2(render);
    auto [displayWidth, displayHeight] = math::float2(display);

    auto widthRatio = renderWidth / displayWidth;
    auto heightRatio = renderHeight / displayHeight;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    float vpx = displayWidth * (1.f - x) / 2.f;
    float vpy = displayHeight * (1.f - y) / 2.f;
    float width = x * displayWidth;
    float height = y * displayHeight;

    D3D12_VIEWPORT viewport = {
        .TopLeftX = vpx,
        .TopLeftY = vpy,
        .Width = width,
        .Height = height,
        .MinDepth = 0.f,
        .MaxDepth = 1.f,
    };

    D3D12_RECT scissor = {
        .left = LONG(vpx),
        .top = LONG(vpy),
        .right = LONG(vpx + width),
        .bottom = LONG(vpy + height),
    };

    return { viewport, scissor };
}
