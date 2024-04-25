#include "stdafx.hpp"

#include "editor/draw.hpp"

#include "draw/draw.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::ed;

using namespace sm::math::literals;

void EditorContext::imgui(graph::FrameGraph& graph, graph::Handle render_target) {
    graph::PassBuilder pass = graph.graphics("ImGui");
    // for (auto& camera : mCameras) {
    //     pass.read(camera->target, camera->camera.name(), graph::Usage::ePixelShaderResource);
    // }

    const ecs::CameraData *it = mCamera.get<ecs::CameraData>();
    pass.read(it->target, "Target", graph::Usage::ePixelShaderResource);

    pass.write(render_target, "Target", graph::Usage::eRenderTarget);

    pass.bind([render_target](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto *cmd = ctx.commands;

        auto rtv = ctx.graph.rtv(render_target);
        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, nullptr);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
    });
}

EditorContext::EditorContext(const render::RenderConfig& config)
    : Super(config)
{
    // push_camera({ 1920, 1080 });

    draw::init_ecs(*this, mSystem);
}

void EditorContext::init() {
    mCamera = mSystem.entity("camera")
        .set<world::ecs::Position>({ math::float3(0.f, 5.f, 10.f) })
        .set<world::ecs::Direction>({ math::float3(0.f, 0.f, 1.f) })
        .set<world::ecs::Camera>({
            .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
            .depth = DXGI_FORMAT_D32_FLOAT,
            .window = mConfig.swapchain.size.as<uint>(),
            .fov = 90._deg
        });
}

void EditorContext::tick(float dt) {
    // for (auto& viewport : mCameras) {
    //     auto& camera = viewport->camera;
    //     camera.tick(dt);
    // }
}

void EditorContext::render() {
    if (recreate_device) {
        recreate_device = false;
        IDeviceContext::recreate_device();
    } else {
        IDeviceContext::render();
    }
}

#if 0
CameraData& EditorContext::add_camera() {
    auto& camera = push_camera({ 800, 600 });

    update_framegraph();

    return camera;
}

CameraData& EditorContext::push_camera(math::uint2 size) {
    draw::ViewportConfig vp { size, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT };
    CameraData data { draw::Camera(fmt::format("Camera {}", mCameras.size()), vp) };

    auto& it = mCameras.emplace_back(sm::make_unique<CameraData>(data));
    input.add_client(&it->camera);
    mActiveCamera = it.get();

    return *mActiveCamera;
}

void EditorContext::erase_camera(size_t index) {
    if (mCameras.size() == 1) return;

    auto& camera = mCameras[index];
    input.erase_client(&camera->camera);

    if (mActiveCamera == camera.get()) {
        mActiveCamera = mCameras[0].get();
    }

    mCameras.erase(mCameras.begin() + index);

    update_framegraph();
}
#endif

void EditorContext::on_create() {
    index = mSrvPool.allocate();

    ImGui_ImplWin32_Init(mConfig.window.get_handle());

    const auto cpu = mSrvPool.cpu_handle(index);
    const auto gpu = mSrvPool.gpu_handle(index);
    ImGui_ImplDX12_Init(getDevice(), int_cast<int>(getSwapChainLength()), getSwapChainFormat(),
                        mSrvPool.get(), cpu, gpu);
}

void EditorContext::on_destroy() {
    mSrvPool.release(index);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

void EditorContext::on_setup() {
    init();
}

void EditorContext::setup_framegraph(graph::FrameGraph& graph) {
    graph::Handle depth;
    graph::Handle target;
    draw::opaque_ecs(graph, target, depth, mCamera, mSystem);

    mCamera.set<ecs::CameraData>({ target, depth });

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
