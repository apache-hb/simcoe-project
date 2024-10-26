#pragma once

#include "render/next/context/core.hpp"

namespace sm::graph {
    class FrameGraph;

    class RenderGraphContextResource : public render::next::IContextResource {
        using Super = render::next::IContextResource;

        void reset() noexcept override final;
        void create() override final;
        void update(render::next::SurfaceInfo info) override final;

        virtual void build(FrameGraph& graph) = 0;

    public:
        RenderGraphContextResource(render::next::CoreContext& context)
            : Super(context)
        { }
    };
}
