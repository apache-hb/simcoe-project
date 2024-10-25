#pragma once

#include "draw/resources/compute.hpp"
#include "draw/resources/imgui.hpp"

namespace sm::draw::next {
    // TODO: make this not reliant on having an HWND
    // only dear imgui requires this for its win32 backend.
    // i think the test engine can work headless so this should be possible
    class DrawContext : public render::next::CoreContext {
        using Super = render::next::CoreContext;

        ImGuiDrawContext *mImGui;
        ComputeContext *mCompute;

    public:
        DrawContext(render::next::ContextConfig config, HWND hwnd);
        ~DrawContext() noexcept;

        void begin();
        void end();
    };
}
