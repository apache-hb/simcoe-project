#pragma once

#include "core/vector.hpp"
#include "render/object.hpp"

namespace sm::render {
    struct CommandList : Object<ID3D12GraphicsCommandList> {
        sm::SmallVector<D3D12_RESOURCE_BARRIER, 4> mBarriers;

        void reset() { Object::reset(); }
        void reset(ID3D12CommandAllocator *allocator, ID3D12PipelineState *pso = nullptr);
        void transition(ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
        void submit_barriers();
    };
}
