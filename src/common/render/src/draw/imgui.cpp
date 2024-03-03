#include "render/draw/imgui.hpp"

#include "render/render.hpp"

using namespace sm;

void draw::draw_imgui(graph::FrameGraph& graph, graph::Handle target) {
    graph::PassBuilder pass = graph.pass("ImGui");
    pass.write(target, graph::Access::eRenderTarget);

    pass.bind([](graph::FrameGraph& graph, render::Context& context) {
        context.render_imgui();
    });
}