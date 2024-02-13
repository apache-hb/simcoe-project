#pragma once

#include "core/map.hpp"
#include "rhi/rhi.hpp"

#include "render.reflect.h"

namespace sm::render {
    class Context;
    class IGraphNode;
    class CommandList;

    using RenderSink = logs::Sink<logs::Category::eRender>;

    static inline constexpr math::float3 kUpVector = { 0.0f, 0.0f, 1.0f };
    static inline constexpr math::float3 kForwardVector = { 0.0f, 1.0f, 0.0f };
    static inline constexpr math::float3 kRightVector = { 1.0f, 0.0f, 0.0f };

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
            CTASSERTF(index != BitMap::eInvalid, "Descriptor heap is full (size = %zu)", m_arena.get_capacity());
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

    class IGraphNode {
        friend class Context;

        // internal node execution
        void build(Context& context);

    protected:
        virtual void create(Context& context) { }
        virtual void destroy(Context& context) { }

        virtual void resize(Context& context, math::uint2 size) { }
        virtual void execute(Context& context) { }

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

        // all graph nodes
        sm::Vector<GraphNodePtr> m_nodes;

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

        void execute_node(IGraphNode& node);
        void destroy_node(IGraphNode& node);
        void resize(math::uint2 size);

        ///
        /// stuff that needs to be deleted at some point
        ///

        void begin_frame();
        void end_frame();
        void present();

        ///
        /// getters and setters
        ///

        rhi::Device& get_rhi() { return m_device; }
        ID3D12Device1 *get_rhi_device() { return m_device.get_device(); }
        const RenderConfig& get_config() const { return m_config; }
        RenderSink& get_logger() { return m_log; }

        SrvArena& get_srv_heap() { return m_srv_arena; }
        RtvArena& get_rtv_heap() { return m_rtv_arena; }
        rhi::CommandListObject& get_direct_commands() { return m_device.get_direct_commands(); }
    };
}
