#pragma once

#include "render/graph/handle.hpp"

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

        D3D12_BARRIER_LAYOUT layout = D3D12_BARRIER_LAYOUT_UNDEFINED;
        D3D12_BARRIER_ACCESS access = D3D12_BARRIER_ACCESS_NO_ACCESS;
        D3D12_BARRIER_SYNC sync = D3D12_BARRIER_SYNC_NONE;
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
