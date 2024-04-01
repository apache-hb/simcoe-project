#include "stdafx.hpp"

#include "editor/draw.hpp"

#include "draw/draw.hpp"

using namespace sm;
using namespace sm::ed;

static void imgui_pass(graph::FrameGraph& graph, graph::Handle target) {
    graph::PassBuilder pass = graph.pass("ImGui");
    pass.write(target, "Target", graph::Access::eRenderTarget);

    pass.bind([](graph::FrameGraph& graph) {
        auto& context = graph.get_context();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *context.mCommandList);
    });
}

EditorContext::EditorContext(const render::RenderConfig& config)
    : Super(config)
{
    cameras.push_back({ draw::Camera("Camera 0") });
}

void EditorContext::tick(float dt) {
    for (auto& viewport : cameras) {
        auto& camera = viewport.camera;
        camera.tick(dt);
    }
}

void EditorContext::on_create() {
    index = mSrvPool.allocate();

    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvPool.cpu_handle(index);
    const auto gpu = mSrvPool.gpu_handle(index);
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainLength), mSwapChainFormat,
                        mSrvPool.get(), cpu, gpu);
}

void EditorContext::on_destroy() {
    mSrvPool.release(index);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void EditorContext::setup_framegraph(graph::FrameGraph& graph) {
    for (auto& viewport : cameras) {
        auto& camera = viewport.camera;
        auto& target = viewport.target;

        draw::opaque(graph, target, camera);
        draw::blit(graph, mSwapChainHandle, target);
    }

    imgui_pass(graph, mSwapChainHandle);
}
