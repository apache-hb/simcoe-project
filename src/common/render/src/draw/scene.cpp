#include "render/draw/scene.hpp"

#include "render/render.hpp"

using namespace sm;

void draw::draw_scene(graph::FrameGraph& graph, graph::Handle target) {
    graph::TextureInfo depth_info = { .name = "Scene/Depth", .size = 1, .format = render::Format::eF32_DEPTH };

    graph::PassBuilder pass = graph.pass("Scene");
    auto depth = pass.create(depth_info, graph::Access::eDepthTarget);
    pass.write(depth, graph::Access::eDepthTarget);
    pass.write(target, graph::Access::eRenderTarget);

    pass.bind([target, depth](graph::FrameGraph& graph, render::Context& context) {
        auto& cmd = context.mCommandList;
        auto rtv = graph.rtv(target);
        auto dsv = graph.dsv(depth);

        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);
        auto dsv_cpu = context.mDsvPool.cpu_handle(dsv);

        cmd->SetGraphicsRootSignature(*context.mPrimitivePipeline.mRootSignature);
        cmd->SetPipelineState(*context.mPrimitivePipeline.mPipelineState);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, &dsv_cpu);

        cmd->RSSetViewports(1, &context.mSceneViewport.mViewport);
        cmd->RSSetScissorRects(1, &context.mSceneViewport.mScissorRect);
        cmd->ClearRenderTargetView(rtv_cpu, render::kClearColour.data(), 0, nullptr);
        cmd->ClearDepthStencilView(dsv_cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        auto [width, height] = context.mSceneSize.as<float>();

        for (auto& primitive : context.mPrimitives) {
            auto model = primitive.mTransform.matrix().transpose();
            auto mvp = context.camera.mvp(width / height, model).transpose();

            cmd->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);
            cmd->IASetVertexBuffers(0, 1, &primitive.mVertexBufferView);
            cmd->IASetIndexBuffer(&primitive.mIndexBufferView);

            cmd->DrawIndexedInstanced(primitive.mIndexCount, 1, 0, 0, 0);
        }
    });
}
