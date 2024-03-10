#include "render/draw/imgui.hpp"

#include "render/render.hpp"

#include "imgui/backends/imgui_impl_dx12.h"

using namespace sm;

void draw::draw_imgui(graph::FrameGraph& graph, graph::Handle target) {
    graph::PassBuilder pass = graph.pass("ImGui");
    pass.write(target, "Target", graph::Access::eRenderTarget);

    pass.bind([](graph::FrameGraph& graph, render::Context& context) {
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *context.mCommandList);
    });
}
