#include "test/render_test_common.hpp"
#include "render/next/viewport.hpp"

#include "graph/graph.hpp"
#include "math/colour.hpp"

#include <directx/d3dx12_core.h>

using namespace sm::math;
using namespace sm::graph;
using namespace sm::render::next;

struct OpaqueDeviceData {
    render::Object<ID3D12RootSignature> signature;
    render::Object<ID3D12PipelineState> pipeline;
};

struct OpaqueHandleData {
    Handle colour;
    Handle depth;
};

static OpaqueHandleData opaque(RenderGraphBuilder& graph, DXGI_FORMAT colour, DXGI_FORMAT depth, math::uint2 resolution, D3D12_VIEWPORT viewport, D3D12_RECT scissor) {
    RenderPassBuilder pass = graph.graphics("Opaque");

    D3D12_RESOURCE_DESC colourDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        colour, resolution.x, resolution.y,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
    );

    D3D12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        depth, resolution.x, resolution.y,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    HandleUseBuilder colourTarget = pass.create("Colour", D3D12_RESOURCE_STATE_RENDER_TARGET, colourDesc);
    HandleUseBuilder depthTarget = pass.create("Depth", D3D12_RESOURCE_STATE_DEPTH_WRITE, depthDesc);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = colourTarget.createRenderTargetView();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthTarget.createDepthStencilView();

    auto& data = graph.data([&] {
        return OpaqueDeviceData { };
    });

    (void)data;

    pass.bind([=](ID3D12GraphicsCommandList* commands) {
        // commands->SetGraphicsRootSignature(root);
        // commands->SetPipelineState(pso);

        commands->RSSetViewports(1, &viewport);
        commands->RSSetScissorRects(1, &scissor);

        commands->OMSetRenderTargets(1, &rtv, false, &dsv);
        commands->ClearRenderTargetView(rtv, kColourClear.data(), 0, nullptr);
        commands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    });

    return { colourTarget, depthTarget };
}

static void blit(RenderGraphBuilder& graph, Handle target, Handle source, D3D12_VIEWPORT viewport, D3D12_RECT scissor) {
    RenderPassBuilder pass = graph.graphics("Blit");
    HandleUseBuilder targetUse = pass.write(target, "Target", D3D12_RESOURCE_STATE_RENDER_TARGET);
    HandleUseBuilder sourceUse = pass.read(source, "Source", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = targetUse.createRenderTargetView();
    D3D12_GPU_DESCRIPTOR_HANDLE srv = sourceUse.createShaderResourceView();

    auto& data = graph.data([&] {
        struct BlitDeviceData { };

        return BlitDeviceData { };
    });

    (void)data;

    pass.bind([=](ID3D12GraphicsCommandList *commands) {
        // commands->SetGraphicsRootSignature(root);
        // commands->SetPipelineState(pso);

        commands->RSSetViewports(1, &viewport);
        commands->RSSetScissorRects(1, &scissor);

        commands->OMSetRenderTargets(1, &rtv, false, nullptr);

        commands->ClearRenderTargetView(rtv, kColourBlack.data(), 0, nullptr);
        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        // commands->IASetVertexBuffers(0, 1, &quad);

        commands->SetGraphicsRootDescriptorTable(0, srv);
        commands->DrawInstanced(4, 1, 0, 0);
    });
}

TEST_CASE("Create RenderGraph") {
    system::create(GetModuleHandle(nullptr));

    WindowContextTest test{30};
    auto& context = test.getContext();
    sm::graph::RenderGraphBuilder graph{context};
    DXGI_FORMAT colourFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;

    auto [viewport, scissor] = next::Viewport::letterbox({ 1920, 1080 }, { 1920, 1080 });
    Handle framebuffer = graph.include("framebuffer", nullptr, D3D12_RESOURCE_STATE_PRESENT);
    auto [colour, depth] = opaque(graph, colourFormat, depthFormat, { 1920, 1080 }, viewport, scissor);
    blit(graph, framebuffer, colour, viewport, scissor);

    SUCCEED("Created RenderGraph successfully");
}
