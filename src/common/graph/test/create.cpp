#include "test/render_test_common.hpp"
#include "render/next/viewport.hpp"

#include "graph/graph.hpp"
#include "math/colour.hpp"

#include <directx/d3dx12_core.h>

using namespace sm::graph;

constexpr math::float4 kColourBlack = math::kColourBlack;
constexpr math::float4 kColourClear = math::kColourClear;

struct OpaqueDeviceData {
    render::Object<ID3D12RootSignature> signature;
    render::Object<ID3D12PipelineState> pipeline;
};

struct OpaqueHandleData {
    Handle colour;
    Handle depth;
};

static OpaqueHandleData opaque(RenderGraph& graph, DXGI_FORMAT colour, DXGI_FORMAT depth, math::uint2 resolution, D3D12_VIEWPORT viewport, D3D12_RECT scissor) {
    RenderPassBuilder pass = graph.graphics("Opaque");
    Handle colourTarget = pass.create("Colour", D3D12_RESOURCE_STATE_RENDER_TARGET, CD3DX12_RESOURCE_DESC::Tex2D(colour, resolution.x, resolution.y));
    Handle depthTarget = pass.create("Depth", D3D12_RESOURCE_STATE_DEPTH_WRITE, CD3DX12_RESOURCE_DESC::Tex2D(depth, resolution.x, resolution.y));

    auto& [root, pso] = graph.data([&] {
        return OpaqueDeviceData { };
    });

    pass.bind([&, viewport, scissor](ID3D12GraphicsCommandList* commands) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = graph.rtv(colourTarget);
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = graph.dsv(depthTarget);

        commands->SetGraphicsRootSignature(root.get());
        commands->SetPipelineState(pso.get());

        commands->RSSetViewports(1, &viewport);
        commands->RSSetScissorRects(1, &scissor);

        commands->OMSetRenderTargets(1, &rtv, false, &dsv);
        commands->ClearRenderTargetView(rtv, kColourClear.data(), 0, nullptr);
        commands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    });

    return { colourTarget, depthTarget };
}

struct BlitDeviceData {
    render::Object<ID3D12RootSignature> signature;
    render::Object<ID3D12PipelineState> pipeline;

    D3D12_VERTEX_BUFFER_VIEW quad;
};

static void blit(RenderGraph& graph, Handle target, Handle source, D3D12_VIEWPORT viewport, D3D12_RECT scissor) {
    RenderPassBuilder pass = graph.graphics("Blit");
    pass.write(target, "Target", D3D12_RESOURCE_STATE_RENDER_TARGET);
    pass.read(source, "Source", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto& [root, pso, quad] = graph.data([&] {
        return BlitDeviceData { };
    });

    pass.bind([&, viewport, scissor](ID3D12GraphicsCommandList *commands) {
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = graph.rtv(target);
        D3D12_GPU_DESCRIPTOR_HANDLE srv = graph.srv(source);

        commands->SetGraphicsRootSignature(root.get());
        commands->SetPipelineState(pso.get());

        commands->RSSetViewports(1, &viewport);
        commands->RSSetScissorRects(1, &scissor);

        commands->OMSetRenderTargets(1, &rtv, false, nullptr);

        commands->ClearRenderTargetView(rtv, kColourBlack.data(), 0, nullptr);
        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        commands->IASetVertexBuffers(0, 1, &quad);

        commands->SetGraphicsRootDescriptorTable(0, srv);
        commands->DrawInstanced(4, 1, 0, 0);
    });
}

TEST_CASE("Create RenderGraph") {
    system::create(GetModuleHandle(nullptr));

    WindowContextTest test{30};
    // auto& context = test.getContext();
    sm::graph::RenderGraph graph;

    auto [viewport, scissor] = next::Viewport::letterbox({ 1920, 1080 }, { 1920, 1080 });
    Handle framebuffer = graph.include(nullptr, D3D12_RESOURCE_STATE_PRESENT);
    auto [colour, depth] = opaque(graph, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, { 1920, 1080 }, viewport, scissor);
    blit(graph, framebuffer, colour, viewport, scissor);

    SUCCEED("Created RenderGraph successfully");
}
