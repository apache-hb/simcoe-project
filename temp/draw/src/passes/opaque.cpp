#include "draw/passes/opaque.hpp"
#include "math/colour.hpp"

#include <directx/d3dx12_barriers.h>

using namespace sm::math;
using namespace sm::draw::next;
using namespace sm::render::next;

struct QuadVertex {
    float2 position;
    float2 uv;
};

constexpr QuadVertex kScreenQuad[] = {
    { { -1.f, -1.f }, { 0.f, 1.f } },
    { { -1.f,  1.f }, { 0.f, 0.f } },
    { {  1.f, -1.f }, { 1.f, 1.f } },
    { {  1.f,  1.f }, { 1.f, 0.f } },
};

void OpaquePass::reset() noexcept {
    DescriptorPool& srv = mContext.getSrvHeap();
    DescriptorPool& rtv = mContext.getRtvHeap();
    DescriptorPool& dsv = mContext.getDsvHeap();

    srv.free(mOpaqueSrv);
    rtv.free(mOpaqueRtv);
    dsv.free(mDepthDsv);

    mOpaqueTarget.reset();
    mDepthTarget.reset();
}

void OpaquePass::create() {
    setup(mContext.getSwapChainSize(), DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
}

void OpaquePass::update(render::next::SurfaceInfo info) {
    reset();
    setup(info.size, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT);
}

void OpaquePass::setup(math::uint2 size, DXGI_FORMAT colour, DXGI_FORMAT depth) {
    auto [width, height] = size;
    ID3D12Device *device = mContext.getDevice();

    DescriptorPool& srv = mContext.getSrvHeap();
    DescriptorPool& rtv = mContext.getRtvHeap();
    DescriptorPool& dsv = mContext.getDsvHeap();

    mOpaqueTarget = TextureResource(mContext, D3D12_RESOURCE_STATE_RENDER_TARGET, CD3DX12_RESOURCE_DESC::Tex2D(colour, width, height), true);
    mDepthTarget = TextureResource(mContext, D3D12_RESOURCE_STATE_DEPTH_WRITE, CD3DX12_RESOURCE_DESC::Tex2D(depth, width, height), true);

    mQuadBuffer = AnyBufferResource(mContext, sizeof(kScreenQuad), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE_UPLOAD);
    mQuadView = D3D12_VERTEX_BUFFER_VIEW { mQuadBuffer.deviceAddress(), sizeof(kScreenQuad), sizeof(QuadVertex) };

    mQuadBuffer.write(kScreenQuad, sizeof(kScreenQuad));

    size_t srvIndex = srv.allocate();

    mOpaqueRtv = rtv.allocateHost();
    mOpaqueSrv = srv.device(srvIndex);
    mDepthDsv = dsv.allocateHost();

    device->CreateRenderTargetView(mOpaqueTarget.get(), nullptr, mOpaqueRtv);
    device->CreateShaderResourceView(mOpaqueTarget.get(), nullptr, srv.host(srvIndex));
    device->CreateDepthStencilView(mDepthTarget.get(), nullptr, mDepthDsv);
}

void OpaquePass::draw(ID3D12GraphicsCommandList *commands) {
    ID3D12Resource *colour = mOpaqueTarget.get();

    const D3D12_RESOURCE_BARRIER cIntoRenderTarget[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            colour,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        ),
    };

    const D3D12_RESOURCE_BARRIER cIntoShaderResource[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            colour,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        ),
    };

    commands->ResourceBarrier(_countof(cIntoRenderTarget), cIntoRenderTarget);

    commands->OMSetRenderTargets(1, &mOpaqueRtv, FALSE, &mDepthDsv);
    commands->ClearRenderTargetView(mOpaqueRtv, kColourClear.data(), 0, nullptr);
    commands->ClearDepthStencilView(mDepthDsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    commands->ResourceBarrier(_countof(cIntoShaderResource), cIntoShaderResource);
}
