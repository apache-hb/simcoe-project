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
    draw::ViewportConfig vp { { 1920, 1080}, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT };
    draw::Camera camera{ "Camera 0", vp };
    cameras.push_back({ camera });
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
    ImGui_ImplDX12_Init(*mDevice, int_cast<int>(mSwapChainConfig.length), mSwapChainConfig.format,
                        mSrvPool.get(), cpu, gpu);
}

void EditorContext::on_destroy() {
    mSrvPool.release(index);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void EditorContext::setup_framegraph(graph::FrameGraph& graph) {
    render::Viewport vp { mSwapChainConfig.size };
    for (auto& viewport : cameras) {
        auto& camera = viewport.camera;
        auto& target = viewport.target;

        draw::opaque(graph, target, camera);
        draw::blit(graph, mSwapChainHandle, target, vp);
    }

    imgui_pass(graph, mSwapChainHandle);
}
