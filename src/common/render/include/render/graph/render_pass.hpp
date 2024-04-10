#pragma once

#include "core/variant.hpp"

#include "render/graph/handle.hpp"
#include "render/heap.hpp"

#include <functional>

#include "render.reflect.h"
#include "graph.reflect.h"

namespace sm::render {
    struct Context;
}

namespace sm::graph {
    class FrameGraph;
    struct RenderPass;

    struct IDeviceData {
        virtual ~IDeviceData() = default;

        virtual void setup(render::Context& context) = 0;
        virtual bool has_type(uint32 index) const = 0;
    };

    struct AccessHandle {
        enum { eRead, eWrite, eCreate } type;
        uint index;
    };

    struct RenderContext {
        render::Context& context;
        FrameGraph& graph;
        RenderPass& pass;
        ID3D12GraphicsCommandList1* commands;

        D3D12_GPU_VIRTUAL_ADDRESS gpu_address(Handle handle) const;

        D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu_handle(Handle handle) const;
        D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu_handle(Handle handle) const;

        D3D12_CPU_DESCRIPTOR_HANDLE uav_cpu_handle(Handle handle) const;
        D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu_handle(Handle handle) const;

        D3D12_CPU_DESCRIPTOR_HANDLE rtv_cpu_handle(Handle handle) const;
        D3D12_GPU_DESCRIPTOR_HANDLE rtv_gpu_handle(Handle handle) const;

        D3D12_CPU_DESCRIPTOR_HANDLE dsv_cpu_handle(Handle handle) const;
        D3D12_GPU_DESCRIPTOR_HANDLE dsv_gpu_handle(Handle handle) const;

        ID3D12Resource* resource(Handle handle) const;
    };

    struct ResourceAccess {
        sm::String name;
        Handle index;
        Usage usage;

        sm::Variant<
            std::monostate,
            D3D12_SHADER_RESOURCE_VIEW_DESC,
            D3D12_UNORDERED_ACCESS_VIEW_DESC,
            D3D12_RENDER_TARGET_VIEW_DESC,
            D3D12_DEPTH_STENCIL_VIEW_DESC
        > view = std::monostate{};

        render::RtvIndex rtv = render::RtvIndex::eInvalid;
        render::DsvIndex dsv = render::DsvIndex::eInvalid;
        render::SrvIndex srv = render::SrvIndex::eInvalid;
        render::SrvIndex uav = render::SrvIndex::eInvalid;
    };

    enum : int {
        eRead = (1 << 0),
        eWrite = (1 << 1),
        eCreate = (1 << 2)
    };

    struct RenderPass {
        uint refcount = 0;
        sm::String name;

        render::CommandListType type;
        bool has_side_effects = false;

        sm::SmallVector<ResourceAccess, 4> creates;
        sm::SmallVector<ResourceAccess, 4> reads;
        sm::SmallVector<ResourceAccess, 4> writes;

        std::function<void(RenderContext&)> execute;

        void foreach(int flags, auto&& fn) {
            if (flags & eRead) {
                for (auto& access : reads) {
                    fn(access);
                }
            }

            if (flags & eWrite) {
                for (auto& access : writes) {
                    fn(access);
                }
            }

            if (flags & eCreate) {
                for (auto& access : creates) {
                    fn(access);
                }
            }
        }

        bool find(int flags, auto&& fn) const {
            if (flags & eRead) {
                for (const auto& access : reads) {
                    if (fn(access)) {
                        return true;
                    }
                }
            }

            if (flags & eWrite) {
                for (const auto& access : writes) {
                    if (fn(access)) {
                        return true;
                    }
                }
            }

            if (flags & eCreate) {
                for (const auto& access : creates) {
                    if (fn(access)) {
                        return true;
                    }
                }
            }

            return false;
        }

        bool is_used() const { return has_side_effects || refcount > 0; }
        bool uses_handle(Handle handle) const;
        bool updates_handle(Handle handle) const;
        Usage get_handle_usage(Handle handle);
        bool depends_on(const RenderPass& other) const;
    };
}
