#pragma once

#include "render/next/context/core.hpp"
#include "render/next/context/resource.hpp"

namespace sm::draw::next {
    class OpaquePass final : public render::next::IContextResource {
        render::next::TextureResource mOpaqueTarget;
        D3D12_CPU_DESCRIPTOR_HANDLE mOpaqueRtv;
        D3D12_GPU_DESCRIPTOR_HANDLE mOpaqueSrv;

        render::next::TextureResource mDepthTarget;
        D3D12_CPU_DESCRIPTOR_HANDLE mDepthDsv;

        render::next::AnyBufferResource mQuadBuffer;
        D3D12_VERTEX_BUFFER_VIEW mQuadView;

        void setup(math::uint2 size, DXGI_FORMAT colour, DXGI_FORMAT depth);

        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;

    public:
        OpaquePass(render::next::CoreContext& context)
            : IContextResource(context)
        { }

        void draw(ID3D12GraphicsCommandList *commands);
    };
}
