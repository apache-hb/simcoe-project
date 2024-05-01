#pragma once

#include "render/graph/handle.hpp"

#include <variant>

#include "render.reflect.h"

namespace sm::graph::events {
    struct DeviceSync {
        render::CommandListType signal;
        render::CommandListType wait;
        FenceHandle fence;
    };

    struct ResourceBarrier {
        CommandListHandle handle;
        sm::Vector<D3D12_RESOURCE_BARRIER> barriers;

        ResourceBarrier(CommandListHandle cmd, sm::Vector<D3D12_RESOURCE_BARRIER> it)
            : handle(cmd)
            , barriers(std::move(it))
        { }

        size_t size() const { return barriers.size(); }
        D3D12_RESOURCE_BARRIER *data() { return barriers.data(); }
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
