#include "stdafx.hpp"

#include "editor/panels/viewport.hpp"
#include "logs/structured/logging.hpp"

#include "editor/draw.hpp"

#include "draw/draw.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::ed;

using namespace sm::math::literals;

void EditorContext::imgui(graph::FrameGraph& graph, graph::Handle target) {
    graph::PassBuilder pass = graph.graphics("ImGui");

    static flecs::query q = mSystem.query_builder<ecs::CameraData>().in().build();
    q.each([&](ecs::CameraData& camera) {
        pass.read(camera.target, "Target", graph::Usage::ePixelShaderResource);
    });

    pass.write(target, "Target", graph::Usage::eRenderTarget);

    pass.bind([target](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto *cmd = ctx.commands;

        auto rtv = ctx.graph.rtv(target);
        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, nullptr);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
    });
}

EditorContext::EditorContext(const render::RenderConfig& config)
    : Super(config)
{
    draw::ecs::initSystems(getWorld(), *this);
    ecs::initWindowComponents(getWorld());
}

void EditorContext::init() {
    mCamera = ecs::addCamera(getWorld(), "Default Camera", { 0.f, 5.f, 10.f }, { 0.f, 0.f, 1.f });

    // whenever a camera is added or removed, the framegraph needs to be rebuilt
    // do this after adding the first camera to avoid rebuilding the framegraph twice
    mCameraObserver = mSystem.observer<const world::ecs::Camera>()
        .event(flecs::OnSet)
        .event(flecs::UnSet)
        .iter([this](flecs::iter& it, const world::ecs::Camera*) {
            LOG_INFO(GlobalLog, "Camera event: {} ({})", it.event().name().c_str(), it.count());
            mFrameGraphDirty = true;
        });
}

void EditorContext::render() {
    bool draw = true;

    if (mDeviceDirty) {
        mDeviceDirty = false;
        draw = false;
        IDeviceContext::recreate_device();
    }

    if (mFrameGraphDirty) {
        mFrameGraphDirty = false;
        update_framegraph();
    }

    // only draw if we didnt recreate the device
    if (draw) {
        IDeviceContext::render();
    }
}

void EditorContext::on_create() {
    index = mSrvPool.allocate();

    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvPool.cpu_handle(index);
    const auto gpu = mSrvPool.gpu_handle(index);
    ImGui_ImplDX12_Init(getDevice(), int_cast<int>(getSwapChainLength()), getSwapChainFormat(),
                        mSrvPool.get(), cpu, gpu);
}

void EditorContext::on_destroy() {
    mCameraObserver.destruct();
    mSrvPool.release(index);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void EditorContext::on_setup() {
    init();
}

void EditorContext::setup_framegraph(graph::FrameGraph& graph) {
    static flecs::query q = mSystem.query<world::ecs::Camera>();
    draw::ecs::DrawData dd { draw::DepthBoundsMode::eEnabled, graph };

    mSystem.defer([&] {
        graph::Handle spotLightVolumes, pointLightVolumes, spotLightData, pointLightData;
        draw::ecs::copyLightData(dd, spotLightVolumes, pointLightVolumes, spotLightData, pointLightData);

        q.each([&](flecs::entity entity, world::ecs::Camera& camera) {
            draw::ecs::WorldData wd { getWorld(), entity };

            graph::Handle depthPrePassTarget;
            graph::Handle lightIndices;
            graph::Handle forwardPlusOpaqueTarget, forwardPlusOpaqueDepth;
            draw::ecs::depthPrePass(wd, dd, depthPrePassTarget);
            draw::ecs::lightBinning(wd, dd, lightIndices, depthPrePassTarget, pointLightVolumes, spotLightVolumes);
            draw::ecs::forwardPlusOpaque(wd, dd, lightIndices, pointLightVolumes, spotLightVolumes, pointLightData, spotLightData, forwardPlusOpaqueTarget, forwardPlusOpaqueDepth);

            entity.set<ecs::CameraData>({ forwardPlusOpaqueTarget, forwardPlusOpaqueDepth });
        });
    });

#if 0
    render::Viewport vp { getSwapChainSize() };
    // graph::Handle point_light_data;
    // graph::Handle spot_light_data;

    // draw::forward_plus::upload_light_data(graph, point_light_data, spot_light_data);
    for (auto& camera : mCameras) {
        // graph::Handle depth_target;
        // graph::Handle light_indices;

        // draw::forward_plus::DrawData dd{camera->camera, draw::forward_plus::DepthBoundsMode::eEnabled};

        // draw::forward_plus::depth_prepass(graph, depth_target, dd);
        // draw::forward_plus::light_binning(graph, light_indices, depth_target, point_light_data, spot_light_data, dd);
        draw::opaque(graph, camera->target, camera->depth, camera->camera);
        game::physics_debug(graph, camera->camera, camera->target);
    }
#endif
    imgui(graph, mSwapChainHandle);
}
