#include "stdafx.hpp"

#include "editor/draw.hpp"

#include "draw/draw.hpp"

using namespace sm;
using namespace sm::ed;

void EditorContext::imgui(graph::FrameGraph& graph, graph::Handle target) {
    graph::PassBuilder pass = graph.pass("ImGui");
    for (auto& [camera, target] : cameras) {
        pass.read(target, camera.name(), graph::Access::ePixelShaderResource);
    }

    pass.write(target, "Target", graph::Access::eRenderTarget);

    pass.bind([target](graph::FrameGraph& graph) {
        auto& context = graph.get_context();
        auto& cmd = context.mCommandList;

        auto rtv = graph.rtv(target);
        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, nullptr);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), *cmd);
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
    for (auto& [camera, target] : cameras) {
        draw::opaque(graph, target, camera);
    }

    imgui(graph, mSwapChainHandle);
}
