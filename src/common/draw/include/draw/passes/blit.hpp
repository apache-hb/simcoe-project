#pragma once

#include "render/next/context/core.hpp"
#include "render/next/context/resource.hpp"

namespace sm::draw::next {
    class BlitPass final : public render::next::IContextResource {
        render::Object<ID3D12RootSignature> mRootSignature;
        render::Object<ID3D12PipelineState> mPipelineState;
        void createShader();

        render::next::TextureResource mBlitTarget;
        D3D12_CPU_DESCRIPTOR_HANDLE mBlitRtv;
        D3D12_GPU_DESCRIPTOR_HANDLE mBlitSrv;
        void createTarget(math::uint2 size, DXGI_FORMAT format);

    public:
        BlitPass(render::next::CoreContext& context, math::uint2 resolution, DXGI_FORMAT format);

        void record(ID3D12GraphicsCommandList *list, UINT index);
        D3D12_GPU_DESCRIPTOR_HANDLE getTargetSrv() const;

        // IContextResource
        void reset() noexcept override;
        void create() override;
        // ~IContextResource
    };
}
