#pragma once

#include "render/graph/handle.hpp"

#include <variant>

#include "render.reflect.h"

namespace sm::graph {
    class FrameGraph;
}

namespace sm::graph::events {
    struct DeviceSync {
        render::CommandListType signal;
        render::CommandListType wait;
        FenceHandle fence;
    };

    struct Transition {
        Handle resource;
        D3D12_RESOURCE_STATES before;
        D3D12_RESOURCE_STATES after;
    };

    struct UnorderedAccess {
        Handle resource;
    };

    struct ResourceBarrier {
        CommandListHandle handle;

        sm::Vector<Transition> transitions;
        sm::Vector<UnorderedAccess> uavs;

        sm::Vector<D3D12_RESOURCE_BARRIER> barriers;

        ResourceBarrier(CommandListHandle cmd, sm::Vector<Transition> transitionBarriers, sm::Vector<UnorderedAccess> uavBarriers)
            : handle(cmd)
            , transitions(std::move(transitionBarriers))
            , uavs(std::move(uavBarriers))
        { }

        void build(FrameGraph& graph);

        size_t size() const { return barriers.size(); }
        const D3D12_RESOURCE_BARRIER *data() const { return barriers.data(); }
    };

    struct OpenCommands {
        CommandListHandle handle;
    };

    struct RecordCommands {
        CommandListHandle handle;
        PassHandle pass;
    };

    struct SubmitCommands {
        CommandListHandle handle;
    };

    using Event = std::variant<
        DeviceSync,
        ResourceBarrier,
        OpenCommands,
        RecordCommands,
        SubmitCommands
    >;
}
