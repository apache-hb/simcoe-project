#pragma once

#include "render/next/context/core.hpp"
#include "render/next/context/resource.hpp"

namespace sm::draw::next {
    class BlitPass final : public render::next::IContextResource {
        using Super = render::next::IContextResource;

        render::Object<ID3D12RootSignature> mRootSignature;
        render::Object<ID3D12PipelineState> mPipelineState;
        void createShader();

        math::uint2 mTargetSize;
        DXGI_FORMAT mTargetFormat;
        render::next::TextureResource mBlitTarget;
        D3D12_CPU_DESCRIPTOR_HANDLE mBlitRtv;
        D3D12_GPU_DESCRIPTOR_HANDLE mBlitSrv;
        void createTarget(math::uint2 size, DXGI_FORMAT format);

        render::next::AnyBufferResource mVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
        void createQuad();

        void setup();

    public:
        BlitPass(render::next::CoreContext& context, math::uint2 resolution, DXGI_FORMAT format);

        void record(ID3D12GraphicsCommandList *list, UINT index, D3D12_GPU_DESCRIPTOR_HANDLE source);
        D3D12_GPU_DESCRIPTOR_HANDLE getTargetSrv() const;
        ID3D12Resource *getTarget() const { return mBlitTarget.get(); }

        // IContextResource
        void reset() noexcept override;
        void create() override;
        // ~IContextResource
    };
}
