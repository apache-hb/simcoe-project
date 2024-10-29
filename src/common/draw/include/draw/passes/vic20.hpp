#pragma once

#include "draw/common/vic20.h"

#include "render/next/context/resource.hpp"

namespace sm::draw::next {
    class Vic20Display final : public render::next::IContextResource {
        using Super = render::next::IContextResource;
        using InfoDeviceBuffer = render::next::MultiBufferResource<shared::Vic20Info>;
        using FrameBufferElement = uint32_t;
        using DeviceFrameBuffer = render::next::BufferResource<FrameBufferElement>;

        render::Object<ID3D12RootSignature> mVic20RootSignature;
        render::Object<ID3D12PipelineState> mVic20PipelineState;
        void createComputeShader();


        InfoDeviceBuffer mInfoBuffer;
        void createConstBuffers(UINT length);
        D3D12_GPU_VIRTUAL_ADDRESS getInfoBufferAddress(UINT index) const noexcept;


        DeviceFrameBuffer mFrameBuffer;
        FrameBufferElement *mFrameBufferPtr = nullptr;
        std::unique_ptr<FrameBufferElement[]> mFrameData;
        void createFrameBuffer();


        render::next::TextureResource mTarget;
        size_t mTargetUAVIndex;
        size_t mTargetSRVIndex;
        math::uint2 mTargetSize;
        void createCompositionTarget();


        void resetDisplayData();
        void createDisplayData(render::next::SurfaceInfo info);
    public:
        Vic20Display(render::next::CoreContext& context, math::uint2 resolution) noexcept;

        void record(ID3D12GraphicsCommandList *list, UINT index);
        ID3D12Resource *getTarget() const noexcept { return mTarget.get(); }
        D3D12_GPU_DESCRIPTOR_HANDLE getTargetSrv() const;

        void write(uint32_t x, uint32_t y, uint8_t colour) noexcept;

        // IContextResource
        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;
        // ~IContextResource
    };
}
