#pragma once

#include "core/core.hpp"

namespace sm::graph {
    // handle to a resource
    struct Handle {
        uint index;

        constexpr auto operator<=>(const Handle&) const = default;
    };

    // a render pass
    struct PassHandle {
        uint index;

        constexpr auto operator<=>(const PassHandle&) const = default;
    };

    struct CommandListHandle {
        uint index;

        bool is_valid() const { return index != UINT_MAX; }
    };

    struct FenceHandle {
        uint index;

        bool is_valid() const { return index != UINT_MAX; }
    };
}
