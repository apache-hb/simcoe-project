#pragma once

#include <directx/d3d12.h>

namespace sm::render::next {
    struct Viewport {
        D3D12_VIEWPORT mViewport;
        D3D12_RECT mScissorRect;

        static Viewport letterbox(math::uint2 display, math::uint2 render);
    };
}
