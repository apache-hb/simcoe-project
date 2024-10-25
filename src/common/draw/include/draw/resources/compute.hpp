#pragma once

#include "render/next/context/core.hpp"

namespace sm::draw::next {
    class ComputeContext final : public render::next::IContextResource {
        using Super = render::next::IContextResource;

        render::Object<ID3D12CommandQueue> mComputeQueue;
        void createCommandQueue();

        std::unique_ptr<render::next::CommandBufferSet> mCommandSet;
        void createCommandList(UINT length);

        std::unique_ptr<render::next::Fence> mComputeFence;
        void createFence();

    public:
        ComputeContext(render::next::CoreContext& ctx);

        void setup(UINT length);

        void begin();
        void end();

        // IContextResource
        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;
        // ~IContextResource
    };
}
