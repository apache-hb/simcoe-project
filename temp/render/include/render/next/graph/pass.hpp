#pragma once

#include "render/next/graph/handle.hpp"

namespace sm::render::next::graph {
    class IRenderPass {
    public:
        virtual ~IRenderPass() = default;

        virtual void execute() = 0;
    };

    class RenderPassBuilder {
    public:
        RenderPassBuilder& reads(Handle handle, D3D12_RESOURCE_STATES state);
        RenderPassBuilder& writes(Handle handle, D3D12_RESOURCE_STATES state);
        RenderPassBuilder& hasSideEffects(bool effects = true);
    };
}
