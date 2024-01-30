#pragma once

#include "core/map.hpp"
#include "rhi/rhi.hpp"

#include "render.reflect.h"

namespace sm::render {
    class Context;
    class IGraphNode;
    class PassAttachment;
    class CommandList;

    using RenderSink = logs::Sink<logs::Category::eRender>;

    static inline constexpr math::float3 kUpVector = { 0.0f, 0.0f, 1.0f };
    static inline constexpr math::float3 kForwardVector = { 0.0f, 1.0f, 0.0f };
    static inline constexpr math::float3 kRightVector = { 1.0f, 0.0f, 0.0f };

    struct CBUFFER_ALIGN CameraBuffer {
        math::float4x4 model;
        math::float4x4 view;
        math::float4x4 projection;
    };

    struct Vertex {
        math::float3 position;
        math::float2 uv;
    };

    template<rhi::DescriptorHeapType::Inner T> requires (rhi::DescriptorHeapType{T}.is_valid())
    class DescriptorArena {
        friend class Context;

        rhi::DescriptorHeap m_heap;
        sm::BitMap m_arena;

        void init(rhi::Device& device, rhi::DescriptorHeapConfig config) {
            m_heap.init(device, config);
            m_arena.resize(config.size);
        }

    public:
        enum Index : size_t { eInvalid = SIZE_MAX };

        ID3D12DescriptorHeap *get_heap() const { return m_heap.get(); }

        rhi::CpuDescriptorHandle cpu_handle(Index index) const {
            return m_heap.cpu_handle(rhi::DescriptorIndex(index));
        }

        rhi::GpuDescriptorHandle gpu_handle(Index index) const {
            return m_heap.gpu_handle(rhi::DescriptorIndex(index));
        }

        Index bind_slot() {
            auto index = m_arena.scan_set_first();
            CTASSERTF(index != BitMap::eInvalid, "Descriptor heap is full (size = %zu)", m_arena.get_total_bits());
            return Index(index);
        }

        void unbind_slot(Index index) {
            m_arena.release(BitMap::Index(index));
        }
    };

    using SrvArena = DescriptorArena<rhi::DescriptorHeapType::eCBV_SRV_UAV>;
    using RtvArena = DescriptorArena<rhi::DescriptorHeapType::eRTV>;

    using SrvHeapIndex = SrvArena::Index;
    using RtvHeapIndex = RtvArena::Index;

    class GraphEdge {

    };

    class NodeInput : GraphEdge {
        rhi::ResourceState m_expects;
    };

    class NodeOutput : GraphEdge {

    };

    using NodeInputPtr = sm::UniquePtr<NodeInput>;

    struct ResourceBinding {
        rhi::DescriptorHeapType heap;
        union {
            SrvHeapIndex srv;
            RtvHeapIndex rtv;
        };

        constexpr ResourceBinding(rhi::DescriptorHeapType heap)
            : heap(heap)
        { }
    };

    class IGraphNode {
        friend class Context;

        sm::Vector<NodeInputPtr> m_inputs;

    protected:
        NodeInput& add_input() {
            auto& input = m_inputs.emplace_back();
            return *input.get();
        }

        virtual void create(Context& context) { }
        virtual void destroy(Context& context) { }

        virtual void resize(Context& context, math::uint2 size) { }

        virtual void build(Context& context) = 0;

    public:
        virtual ~IGraphNode() = default;
    };

    class IRenderNode : public IGraphNode {
        // TODO: render targets and depth stencil
    };

    class Context {
        using GraphNodePtr = sm::UniquePtr<IGraphNode>;

        RenderConfig m_config;
        RenderSink m_log;

        SM_UNUSED rhi::Factory& m_factory;
        rhi::Device m_device;

        SrvArena m_srv_arena;
        RtvArena m_rtv_arena;

        // the render graph is just a tree of nodes like an ast
        sm::Vector<GraphNodePtr> m_nodes;
        sm::Map<NodeInput*, IGraphNode*> m_edges;
        sm::Map<IGraphNode*, bool> m_executed;

        void execute_inner(IGraphNode& node);

        void create_node(IGraphNode& node);

    public:
        SM_NOCOPY(Context)

        Context(RenderConfig config, rhi::Factory& factory);
        ~Context();

        template<std::derived_from<IGraphNode> T>
        T& add_node(auto&&... args) requires (std::is_constructible_v<T, decltype(args)...>) {
            T *node = new T(std::forward<decltype(args)>(args)...);
            create_node(*node);
            return *node;
        }

        void connect(NodeInput* dst, IGraphNode* src);

        void execute_node(IGraphNode& node);
        void destroy_node(IGraphNode& node);

        void resize(math::uint2 size);

        void begin_frame();
        void end_frame();
        void present();

        rhi::Device& get_rhi() { return m_device; }
        ID3D12Device1 *get_rhi_device() { return m_device.get_device(); }
        const RenderConfig& get_config() const { return m_config; }
        RenderSink& get_logger() { return m_log; }

        SrvArena& get_srv_heap() { return m_srv_arena; }
        RtvArena& get_rtv_heap() { return m_rtv_arena; }
        rhi::CommandListObject& get_direct_commands() { return m_device.get_direct_commands(); }
    };
}
