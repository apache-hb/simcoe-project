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
}
