#pragma once

#include "core/span.hpp"

#include "render/object.hpp"

namespace sm::render {
    struct CommandList : Object<ID3D12GraphicsCommandList> {
        void reset() { Object::reset(); }
        void reset(ID3D12CommandAllocator *allocator, ID3D12PipelineState *pso = nullptr);
        void submit_barriers(sm::View<D3D12_RESOURCE_BARRIER> barriers);
    };
}
