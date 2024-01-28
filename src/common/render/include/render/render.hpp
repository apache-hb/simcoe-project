#pragma once

#include "rhi/rhi.hpp"

#include "render.reflect.h"

namespace sm::render {
    class Context;
    class ICommandBundle;
    class PassAttachment;
    class CommandList;

    class Texture { };

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

    class PipelineState {
        friend class Context;

        rhi::RootSignatureObject m_signature;
        rhi::PipelineStateObject m_pipeline;
    };

    // render passes are stateless, and only store commands and resource
    // input/output dependencies
    // the graph is just a DAG of render passes connected by edges

    class GraphEdge {

    };

    class NodeInput : GraphEdge {
        rhi::ResourceState m_expects;
    };

    class NodeOutput : GraphEdge {
        rhi::ResourceState m_produces;
    };

    using NodeInputPtr = sm::UniquePtr<NodeInput>;
    using NodeOutputPtr = sm::UniquePtr<NodeOutput>;

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

    class CommandList {
        friend class Context;

        rhi::CommandListObject m_commands;
        rhi::CpuDescriptorHandle m_current_rtv;

    public:
        void bind_rtv(rhi::CpuDescriptorHandle rtv) {
            if (m_current_rtv.ptr == rtv.ptr)
                return;

            m_current_rtv = rtv;
            m_commands->OMSetRenderTargets(1, &rtv, false, nullptr);
        }
    };

    class ICommandBundle {
        friend class Context;

        sm::Vector<NodeInputPtr> m_inputs;
        sm::Vector<NodeOutputPtr> m_outputs;

        sm::Vector<sm::UniquePtr<ResourceBinding>> m_bindings;

        void bind_resources(Context& context);
        void unbind_resources(Context& context);

    protected:
        NodeInput& add_input() {
            auto& input = m_inputs.emplace_back();
            return *input.get();
        }

        NodeOutput& add_output() {
            auto& output = m_outputs.emplace_back();
            return *output.get();
        }

        constexpr SrvHeapIndex& srv_local() {
            // TODO: might return reference to array element that gets invalidated
            auto& binding = m_bindings.emplace_back(new ResourceBinding(rhi::DescriptorHeapType::eCBV_SRV_UAV));
            binding->srv = SrvHeapIndex::eInvalid;
            return binding->srv;
        }

        constexpr RtvHeapIndex& rtv_local() {
            auto& binding = m_bindings.emplace_back(new ResourceBinding(rhi::DescriptorHeapType::eRTV));
            binding->rtv = RtvHeapIndex::eInvalid;
            return binding->rtv;
        }

    public:
        virtual ~ICommandBundle() = default;

        virtual void create(Context& context) { }
        virtual void destroy(Context& context) { }

        virtual void build(Context& context) = 0;
    };

    class IRenderCommands : public ICommandBundle {
        // TODO: render targets and depth stencil
    };

    class Context {
        using GraphNodePtr = sm::UniquePtr<ICommandBundle>;

        RenderConfig m_config;

        rhi::Factory& m_factory;
        rhi::Device m_device;

        SrvArena m_srv_arena;
        RtvArena m_rtv_arena;

        sm::Vector<GraphNodePtr> m_nodes;

        void create_node(ICommandBundle& node);

    public:
        Context(RenderConfig config, rhi::Factory& factory);
        ~Context();

        template<std::derived_from<ICommandBundle> T>
        T& add_node(auto&&... args) requires (std::is_constructible_v<T, decltype(args)...>) {
            T *node = new T(std::forward<decltype(args)>(args)...);
            create_node(*node);
            return *node;
        }

        void connect(NodeInput& dst, NodeOutput& src);

        void execute_node(ICommandBundle& node);
        void destroy_node(ICommandBundle& node);

        rhi::Device& get_rhi() { return m_device; }
        ID3D12Device1 *get_rhi_device() { return m_device.get_device(); }
        const RenderConfig& get_config() const { return m_config; }

        SrvArena& get_srv_heap() { return m_srv_arena; }
        RtvArena& get_rtv_heap() { return m_rtv_arena; }
        rhi::CommandListObject& get_direct_commands() { return m_device.get_commands(); }
    };
}
