#pragma once

#include "render/render.hpp" // IWYU pragma: export

#include "graph.reflect.h"

namespace sm::graph {
    class IRenderObject;
    class IRenderPass;
    class IResource;

    class IGraphNode;
    class ResourceNode;
    class PassNode;

    class PassBuilder;
    class ResourceBuilder;
    class FrameGraph;

    using ResourceState = D3D12_RESOURCE_STATES;
    using Barrier = D3D12_RESOURCE_BARRIER;

    class ResourceBuilder {
        PassBuilder& mBuilder;
        ResourceId mResource;

    public:
        ResourceBuilder(PassBuilder& builder, ResourceId resource)
            : mBuilder(builder)
            , mResource(resource)
        { }

        ResourceId read(ResourceState state);
        ResourceId write(ResourceState state);

        operator ResourceId() const { return mResource; }
    };

    class PassBuilder {
        FrameGraph& mGraph;
        PassNode& mNode;

    public:
        PassBuilder(FrameGraph& graph, PassNode& node)
            : mGraph(graph)
            , mNode(node)
        { }

        // produces side effects the frame graph cant observe
        // disallows culling of the pass
        void volatile_effects();

        ResourceId read(ResourceId id, ResourceState state);
        ResourceId write(ResourceId id, ResourceState state);

        template<typename T, typename... A>
        ResourceBuilder produce(sm::StringView name, ResourceState state, A&&... args);
    };

    /// user facing handles

    class IRenderObject {
    public:
        virtual ~IRenderObject() = default;

        // called on first frame this pass is used
        // setup whatever data this node may require
        // shaders, pipeline objects, etc
        virtual void create(FrameGraph& graph, render::Context& context) = 0;

        // called when destroying the frame graph or
        // when the pass is being recreated on device loss
        virtual void destroy(FrameGraph& graph, render::Context& context) = 0;
    };

    class IRenderPass : public IRenderObject {
    public:
        virtual ~IRenderPass() = default;

        // TODO: what is the distinction between build/execute
        // or build/ctors

        // called when the frame graph is being compiled
        virtual void build(FrameGraph& graph, render::Context& context) = 0;

        // called when this pass is executed
        virtual void execute(FrameGraph& graph, render::Context& context) = 0;
    };

    class IResource : public IRenderObject {
    public:
        virtual ~IResource() = default;

        virtual Barrier transition(ResourceState before, ResourceState after) = 0;
    };

    template<typename T>
    concept IsRenderPass = std::derived_from<T, IRenderPass>;

    template<typename T>
    concept IsResource = std::derived_from<T, IResource>;

    /// graph nodes

    struct NodeConfig {
        sm::StringView name;
        uint id;
        render::Context& context;
    };

    class IGraphNode {
        friend FrameGraph;

        sm::String mName;
        uint mNodeId;
        uint mRefCount = 0;

        render::Context& mContext;

    public:
        IGraphNode(const NodeConfig& config)
            : mName(config.name)
            , mNodeId(config.id)
            , mContext(config.context)
        { }

        sm::StringView get_name() const { return mName; }
        uint get_id() const { return mNodeId; }
        uint get_refcount() const { return mRefCount; }
    };

    class ResourceNode final : IGraphNode {
        friend FrameGraph;

        uint mVersion;
        IResource *mResource;
        ResourceType mType;

        PassNode *mProducer;
        PassNode *mLastUser;

        Barrier transition(ResourceState before, ResourceState after);
        void create(FrameGraph& graph, render::Context& context);
        void destroy(FrameGraph& graph, render::Context& context);

    public:
        ResourceNode(const NodeConfig& config, uint version, IResource *resource, ResourceType type)
            : IGraphNode(config)
            , mVersion(version)
            , mResource(resource)
            , mType(type)
        { }

        uint get_version() const { return mVersion; }
        IResource *get_resource() const { return mResource; }

        ResourceType get_type() const { return mType; }
        bool is_imported() const { return mType == ResourceType::eImported; }
        bool is_transient() const { return mType == ResourceType::eTransient; }
    };

    struct ResourceAccess {
        uint id;
        ResourceState state;
    };

    class PassNode final : IGraphNode {
        friend FrameGraph;

        sm::SmallVector<ResourceAccess, 4> mReads;
        sm::SmallVector<ResourceAccess, 4> mWrites;
        sm::SmallVector<ResourceAccess, 4> mCreates;
        bool mHasSideEffects = false;

    public:
        SM_NOCOPY(PassNode)

        PassNode(const NodeConfig& config)
            : IGraphNode(config)
        { }

        void create(FrameGraph& graph, render::Context& context);
        void destroy(FrameGraph& graph, render::Context& context);
        void execute(FrameGraph& graph, render::Context& context);

        bool has_side_effects() const { return mHasSideEffects; }
    };

    /// the graph

    class FrameGraph {
        sm::Vector<ResourceNode> mResources;
        sm::Vector<PassNode> mPasses;
        sm::Vector<ResourceAccess> mImports;

        render::Context& mContext;

        ResourceId new_resource(sm::StringView name, ResourceState state, IResource *resource, ResourceType type);

    public:
        FrameGraph(render::Context& context);

        template<IsResource T, typename... A> requires std::constructible_from<T, A...>
        ResourceId import(sm::StringView name, ResourceState state, A&&... args) {
            return new_resource(name, state, new T(std::forward<A>(args)...), ResourceType::eImported);
        }

        template<IsRenderPass T, typename... A> requires std::constructible_from<T, PassBuilder&, A...>
        T& pass(sm::StringView name, A&&... args) {
            auto id = mPasses.size();
            NodeConfig config = {name, int_cast<uint>(id), mContext};
            mPasses.emplace_back(config, std::forward<A>(args)...);
            return mPasses.back();
        }

        void compile();
        void execute(render::Context& context);
    };
}
