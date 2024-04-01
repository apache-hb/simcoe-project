#include "stdafx.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::render;

void Context::update_display_viewport() {
    auto [renderWidth, renderHeight] = mSceneSize.as<float>();
    auto [displayWidth, displayHeight] = mSwapChainSize.as<float>();

    auto widthRatio = renderWidth / displayWidth;
    auto heightRatio = renderHeight / displayHeight;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    float viewport_x = displayWidth * (1.f - x) / 2.f;
    float viewport_y = displayHeight * (1.f - y) / 2.f;
    float viewport_width = x * displayWidth;
    float viewport_height = y * displayHeight;

    mPresentViewport.mViewport = {
        .TopLeftX = viewport_x,
        .TopLeftY = viewport_y,
        .Width = viewport_width,
        .Height = viewport_height,
        .MinDepth = 0.f,
        .MaxDepth = 1.f,
    };

    mPresentViewport.mScissorRect = {
        .left = LONG(viewport_x),
        .top = LONG(viewport_y),
        .right = LONG(viewport_x + viewport_width),
        .bottom = LONG(viewport_y + viewport_height),
    };
}
