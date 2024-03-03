#include "render/draw/present.hpp"

#include "render/render.hpp"

using namespace sm;

void draw::draw_present(graph::FrameGraph& graph, graph::Handle target, graph::Handle source) {
    graph::PassBuilder pass = graph.pass("Present");
    pass.read(source, graph::Access::ePixelShaderResource);
    pass.write(target, graph::Access::eRenderTarget);

    pass.bind([target, source](graph::FrameGraph& graph, render::Context& context) {
        auto& cmd = context.mCommandList;
        auto rtv = graph.rtv(target);
        auto srv = graph.srv(source);

        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);
        auto srv_gpu = context.mSrvPool.gpu_handle(srv);

        cmd->SetGraphicsRootSignature(*context.mBlitPipeline.signature);
        cmd->SetPipelineState(*context.mBlitPipeline.pso);

        cmd->RSSetViewports(1, &context.mPresentViewport.mViewport);
        cmd->RSSetScissorRects(1, &context.mPresentViewport.mScissorRect);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, nullptr);

        cmd->ClearRenderTargetView(rtv_cpu, render::kColourBlack.data(), 0, nullptr);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cmd->IASetVertexBuffers(0, 1, &context.mScreenQuad.mVertexBufferView);

        cmd->SetGraphicsRootDescriptorTable(0, srv_gpu);
        cmd->DrawInstanced(4, 1, 0, 0);
    });
}
