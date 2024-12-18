#pragma once

#include "render/next/surface/surface.hpp"

namespace sm::render::next {
    class WindowSwapChainFactory final : public ISwapChainFactory {
        HWND mWindow;

    public:
        ISwapChain *createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) override;

        WindowSwapChainFactory(HWND window);
    };
}
