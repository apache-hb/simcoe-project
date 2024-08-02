#pragma once

#include "core/core.hpp"

#include <functional>

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

        constexpr auto operator<=>(const CommandListHandle&) const = default;
        bool is_valid() const { return index != UINT_MAX; }
    };

    struct FenceHandle {
        uint index;

        constexpr auto operator<=>(const FenceHandle&) const = default;
        bool is_valid() const { return index != UINT_MAX; }
    };

    struct RenderPassHandle {
        uint index;

        constexpr auto operator<=>(const RenderPassHandle&) const = default;
        bool is_valid() const { return index != UINT_MAX; }
    };
}

template<>
struct std::hash<sm::graph::Handle> {
    size_t operator()(const sm::graph::Handle& handle) const {
        return std::hash<sm::uint>{}(handle.index);
    }
};

template<>
struct std::hash<sm::graph::PassHandle> {
    size_t operator()(const sm::graph::PassHandle& handle) const {
        return std::hash<sm::uint>{}(handle.index);
    }
};

template<>
struct std::hash<sm::graph::CommandListHandle> {
    size_t operator()(const sm::graph::CommandListHandle& handle) const {
        return std::hash<sm::uint>{}(handle.index);
    }
};

template<>
struct std::hash<sm::graph::FenceHandle> {
    size_t operator()(const sm::graph::FenceHandle& handle) const {
        return std::hash<sm::uint>{}(handle.index);
    }
};

template<>
struct std::hash<sm::graph::RenderPassHandle> {
    size_t operator()(const sm::graph::RenderPassHandle& handle) const {
        return std::hash<sm::uint>{}(handle.index);
    }
};
