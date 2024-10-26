#include "test/render_test_common.hpp"

#include "graph/graph.hpp"

#include <directx/d3dx12_core.h>

using namespace sm::graph;

static void opaque(RenderGraph& graph, Handle& colour, Handle& depth, DXGI_FORMAT colourFormat, DXGI_FORMAT depthFormat, math::uint2 resolution) {
    RenderPass pass = graph.graphics("Opaque");
    colour = pass.create("Colour", D3D12_RESOURCE_STATE_RENDER_TARGET, CD3DX12_RESOURCE_DESC::Tex2D(colourFormat, resolution.x, resolution.y));
    depth = pass.create("Depth", D3D12_RESOURCE_STATE_DEPTH_WRITE, CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, resolution.x, resolution.y));
}

static void blit(RenderGraph& graph, Handle target, Handle source) {
    RenderPass pass = graph.graphics("Blit");
    pass.read(source, "Source", D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    pass.write(target, "Target", D3D12_RESOURCE_STATE_RENDER_TARGET);
}

TEST_CASE("Create RenderGraph") {
    WindowBaseTest test{30};
    sm::graph::RenderGraph graph;
    Handle colour;
    Handle depth;
    Handle framebuffer = graph.framebuffer();
    opaque(graph, colour, depth, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, { 1920, 1080 });
    blit(graph, framebuffer, colour);
    SUCCEED("Created RenderGraph successfully");
}
