#include "stdafx.hpp"

#include "render/draw/draw.hpp"

#include "render/render.hpp"

using namespace sm;

void draw::imgui(graph::FrameGraph& graph, graph::Handle target) {
    graph::PassBuilder pass = graph.pass("ImGui");
    pass.write(target, "Target", graph::Access::eRenderTarget);

    pass.bind([](graph::FrameGraph& graph, render::Context& context) {
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *context.mCommandList);
    });
}
