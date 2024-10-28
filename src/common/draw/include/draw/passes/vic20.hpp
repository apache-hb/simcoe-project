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


        render::next::BufferResource mVic20FrameBuffer;
        void createFrameBuffer();


        render::next::TextureResource mVic20Target;
        size_t mVic20TargetUAVIndex;
        math::uint2 mTargetSize;
        void createCompositionTarget(math::uint2 size);


        void resetDisplayData();
        void createDisplayData(render::next::SurfaceInfo info);
    public:
        Vic20Display(render::next::CoreContext& context) noexcept;

        void record(ID3D12GraphicsCommandList *list, UINT index);

        // IContextResource
        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;
        // ~IContextResource
    };
}
