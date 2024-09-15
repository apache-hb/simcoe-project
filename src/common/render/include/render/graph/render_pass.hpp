#pragma once

#include "render/graph/handle.hpp"

#include "core/adt/small_vector.hpp"

#include <functional>

#include "render.reflect.h"
#include "graph.reflect.h"

namespace sm::render {
    struct IDeviceContext;
}

namespace sm::graph {
    class FrameGraph;
    struct RenderPass;

    struct IDeviceData {
        virtual ~IDeviceData() = default;

        virtual void setup(render::IDeviceContext& context) = 0;
        virtual bool has_type(uint32 index) const = 0;
    };

    struct AccessHandle {
        enum { eRead, eWrite, eCreate } type;
        uint index;
    };

    struct RenderContext {
        render::IDeviceContext& context;
        FrameGraph& graph;
        RenderPass& pass;
        ID3D12GraphicsCommandList1* commands;
    };

    struct ResourceAccess {
        sm::String name;
        Handle index;
        Usage usage;
        D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_COMMON;

        D3D12_BARRIER_LAYOUT layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
        D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_NO_ACCESS;
        D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;

        bool isReadState() const;
        D3D12_RESOURCE_STATES getStateFlags() const;
    };

    constexpr bool isReadOnlyState(D3D12_RESOURCE_STATES states) {
        return (states & ~D3D12_RESOURCE_STATE_GENERIC_READ) == 0;
    }

    constexpr D3D12_RESOURCE_STATES getStateFromUsage(Usage usage) {
        using enum Usage::Inner;
        switch (usage) {
        case ePresent: return D3D12_RESOURCE_STATE_PRESENT;
        case eUnknown:
            CT_NEVER("Unknown resource state");
        default:
            CT_NEVER("Unknown resource state");

        case eComputeShaderResource: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case ePixelShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

        case eTextureRead: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case eTextureWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        case eRenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;

        case eCopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case eCopyTarget: return D3D12_RESOURCE_STATE_COPY_DEST;

        case eBufferRead: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case eBufferWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        case eDepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
        case eDepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;

        case eIndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
        case eVertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
    }

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

        std::vector<ResourceAccess> creates;
        std::vector<ResourceAccess> reads;
        std::vector<ResourceAccess> writes;

        std::function<void(RenderContext&)> execute;

        void foreach(this auto& self, int flags, auto&& fn) {
            if (flags & eRead) {
                for (auto& access : self.reads) {
                    fn(access);
                }
            }

            if (flags & eWrite) {
                for (auto& access : self.writes) {
                    fn(access);
                }
            }

            if (flags & eCreate) {
                for (auto& access : self.creates) {
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

        bool isUsed() const { return has_side_effects || refcount > 0; }
        bool uses_handle(Handle handle) const;
        bool updates_handle(Handle handle) const;
        Usage get_handle_usage(Handle handle) const;
        bool depends_on(const RenderPass& other) const;
        ResourceAccess getHandleAccess(Handle handle) const;
    };
}
