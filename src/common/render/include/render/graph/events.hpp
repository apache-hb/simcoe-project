#pragma once

#include "core/variant.hpp"

#include "render/graph/handle.hpp"

#include "render.reflect.h"

namespace sm::graph {
    struct CommandListHandle {
        uint index;

        bool is_valid() const { return index != UINT_MAX; }
    };
}

namespace sm::graph::events {
    struct DeviceSync {
        render::CommandListType signal;
        render::CommandListType wait;
    };

    struct ResourceBarrier {
        struct Transition {
            Handle handle;
            D3D12_RESOURCE_STATES before;
            D3D12_RESOURCE_STATES after;
        };

        CommandListHandle handle;
        sm::Vector<Transition> barriers;
        sm::Vector<D3D12_RESOURCE_BARRIER> raw;

        ResourceBarrier(CommandListHandle cmd, sm::Vector<Transition> it)
            : handle(cmd)
            , barriers(std::move(it))
            , raw(barriers.size())
        { }

        size_t size() const { return barriers.size(); }
        D3D12_RESOURCE_BARRIER *data() { return raw.data(); }
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

    using Event = sm::Variant<
        DeviceSync,
        ResourceBarrier,
        OpenCommands,
        RecordCommands,
        SubmitCommands
    >;
}
