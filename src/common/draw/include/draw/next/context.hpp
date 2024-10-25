#pragma once

#include "draw/resources/compute.hpp"
#include "draw/resources/imgui.hpp"

namespace sm::draw::next {
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
