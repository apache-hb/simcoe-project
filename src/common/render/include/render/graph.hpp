#pragma once

#include "render/render.hpp" // IWYU pragma: export

#include "graph.reflect.h" // IWYU pragma: export

namespace sm::graph {
    class IRenderPass;

    class PassBuilder;
    class FrameGraph;
    class Handle; // resource handle

    using ResourceState = D3D12_RESOURCE_STATES;
    using Barrier = D3D12_RESOURCE_BARRIER;

    struct Read {
        ResourceState state;
    };

    struct Write {
        ResourceState state;
    };

    struct Create {
        ResourceState state;
        render::Format format; // format of the resource

        uint width = 1; // 1 for full resolution, 2 for half
        uint height = 1; // 1 for full resolution, 2 for half
    };

    class IRenderPass {
    protected:
        render::Context& ctx;
        FrameGraph& graph;

    public:
        IRenderPass(render::Context& ctx, FrameGraph& fg)
            : ctx(ctx)
            , graph(fg)
        { }

        virtual void create() { }
        virtual void destroy() { }

        virtual void build(PassBuilder& builder) = 0;
        virtual void execute() = 0;
    };

    class Resource {

    };

    class PassBuilder {
        render::Context& mContext;
        IRenderPass& mPass;
        FrameGraph& mGraph;

    public:
        PassBuilder(render::Context& context, IRenderPass& pass, FrameGraph& graph)
            : mContext(context)
            , mPass(pass)
            , mGraph(graph)
        { }

        Handle read(sm::StringView name, ResourceState state);
        Handle write(sm::StringView name, ResourceState state);
        Handle create(sm::StringView name, ResourceState state, uint width, uint height, render::Format format);
    };

    class FrameGraph {
        sm::Vector<IRenderPass*> mPasses;
        sm::Vector<Resource*> mResources;

        render::Context& mContext;

    public:
        FrameGraph(render::Context& context)
            : mContext(context)
        { }

        void add(IRenderPass* pass);

        void build();
        void execute();
    };
}
