#pragma once

#include "draw/resources/compute.hpp"
#include "draw/resources/imgui.hpp"
#include "draw/passes/vic20.hpp"

namespace sm::draw::next {
    // TODO: make this not reliant on having an HWND
    // only dear imgui requires this for its win32 backend.
    // i think the test engine can work headless so this should be possible
    class DrawContext : public render::next::CoreContext {
        using Super = render::next::CoreContext;

        ImGuiDrawContext *mImGui;
        ComputeContext *mCompute;

        Vic20Display *mVic20;

    public:
        DrawContext(render::next::ContextConfig config, HWND hwnd, math::uint2 resolution);

        void begin();
        void end();

        D3D12_GPU_DESCRIPTOR_HANDLE getVic20TargetSrv() const noexcept {
            return mVic20->getTargetSrv();
        }

        void write(uint8_t x, uint8_t y, uint8_t colour) {
            mVic20->write(x, y, colour);
        }
    };
}
