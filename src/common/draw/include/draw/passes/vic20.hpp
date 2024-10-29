#pragma once

#include "draw/common/vic20.h"

#include "draw/resources/compute.hpp"
#include "draw/resources/imgui.hpp"
#include "render/next/context/resource.hpp"

namespace sm::draw::next {
    class Vic20Display final : public render::next::IContextResource {
        render::Object<ID3D12RootSignature> mVic20RootSignature;
        render::Object<ID3D12PipelineState> mVic20PipelineState;
        void createComputeShader();


        render::next::BufferResource mVic20InfoBuffer;
        void createConstBuffers(UINT length);
        D3D12_GPU_VIRTUAL_ADDRESS getInfoBufferAddress(UINT index) const noexcept;


        render::next::BufferResource mFrameBuffer;
        void *mFrameBufferPtr = nullptr;
        std::unique_ptr<uint32_t[]> mFrameData = std::make_unique<uint32_t[]>(VIC20_FRAMEBUFFER_SIZE);
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

        void write(uint8_t x, uint8_t y, uint8_t colour) noexcept;

        // IContextResource
        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;
        // ~IContextResource
    };
}
