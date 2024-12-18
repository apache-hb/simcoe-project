#pragma once

#include "render/next/context/core.hpp"

namespace sm::draw::next {
    class ComputeContext final : public render::next::IContextResource {
        using Super = render::next::IContextResource;

        render::Object<ID3D12CommandQueue> mComputeQueue;
        void createCommandQueue();

        std::unique_ptr<render::next::CommandBufferSet> mCommandSet;
        void createCommandList(UINT length);

        render::Object<ID3D12Fence> mComputeFence;
        uint64_t mFenceValue = 0;
        void createFence();

    public:
        ComputeContext(render::next::CoreContext& ctx);

        void setup(UINT length);

        void begin(UINT index);
        void end();

        /// @brief add a gpu timeline sync point to the incoming queue
        /// Blocks @p queue until the compute queue has completed its work.
        /// @param queue the queue to block
        void blockQueueUntil(ID3D12CommandQueue *queue);

        /// @brief sync the compute queue with the incoming queue
        void waitOnQueue(ID3D12CommandQueue *queue);

        ID3D12GraphicsCommandList *getCommandList() const noexcept { return mCommandSet->get(); }

        // IContextResource
        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;
        // ~IContextResource
    };
}
