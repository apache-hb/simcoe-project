#include "stdafx.hpp"

#include "render/draw/draw.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::math;

static void draw_node(render::Context& context, uint16 index, const float4x4& parent) {
    float aspect_ratio = float(context.mSceneSize.width) / float(context.mSceneSize.height);
    const auto& node = context.mWorld.info.nodes[index];

    auto model = (parent * node.transform.matrix());
    float4x4 mvp = context.camera.mvp(aspect_ratio, model.transpose()).transpose();

    auto& cmd = context.mCommandList;
    cmd->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

    for (uint16 i : node.objects) {
        const auto& object = context.mMeshes[i];
        cmd->IASetVertexBuffers(0, 1, &object.mVertexBufferView);
        cmd->IASetIndexBuffer(&object.mIndexBufferView);

        cmd->DrawIndexedInstanced(object.mIndexCount, 1, 0, 0, 0);
    }

    for (uint16 i : node.children) {
        draw_node(context, i, model);
    }
}

void draw::opaque(graph::FrameGraph& graph, graph::Handle& target) {
    auto& ctx = graph.get_context();
    graph::ResourceInfo depth_info = {
        .size = graph.render_size(),
        .format = ctx.mDepthFormat,
        .clear = graph::clear_depth(1.f)
    };

    graph::ResourceInfo target_info = {
        .size = graph.render_size(),
        .format = ctx.mSceneFormat,
        .clear = graph::clear_colour(render::kClearColour)
    };

    graph::PassBuilder pass = graph.pass("Scene");
    target = pass.create(target_info, "Target", graph::Access::eRenderTarget);
    auto depth = pass.create(depth_info, "Depth", graph::Access::eDepthTarget);

    pass.bind([target, depth](graph::FrameGraph& graph, render::Context& context) {
        auto& cmd = context.mCommandList;
        auto rtv = graph.rtv(target);
        auto dsv = graph.dsv(depth);

        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);
        auto dsv_cpu = context.mDsvPool.cpu_handle(dsv);

        cmd->SetGraphicsRootSignature(*context.mPrimitivePipeline.signature);
        cmd->SetPipelineState(*context.mPrimitivePipeline.pso);

        cmd->RSSetViewports(1, &context.mSceneViewport.mViewport);
        cmd->RSSetScissorRects(1, &context.mSceneViewport.mScissorRect);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, &dsv_cpu);
        cmd->ClearRenderTargetView(rtv_cpu, render::kClearColour.data(), 0, nullptr);
        cmd->ClearDepthStencilView(dsv_cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        draw_node(context, context.mWorld.info.root_node, math::float4x4::identity());
    });
}
