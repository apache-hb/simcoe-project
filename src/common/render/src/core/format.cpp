#include "stdafx.hpp"

#include "render/core/format.hpp"

#include <fmt/ranges.h>

sm::String sm::render::format(D3D12_RESOURCE_STATES states) {
    struct StateFlag {
        D3D12_RESOURCE_STATES state;
        const char *name;

        constexpr StateFlag(D3D12_RESOURCE_STATES state, const char *name)
            : state(state)
            , name(name + sizeof("D3D12_RESOURCE_STATE_") - 1)
        { }
    };

#define FLAG_NAME(x) { x, #x }

    static constexpr StateFlag kStates[] = {
        FLAG_NAME(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
        FLAG_NAME(D3D12_RESOURCE_STATE_INDEX_BUFFER),
        FLAG_NAME(D3D12_RESOURCE_STATE_RENDER_TARGET),
        FLAG_NAME(D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        FLAG_NAME(D3D12_RESOURCE_STATE_DEPTH_WRITE),
        FLAG_NAME(D3D12_RESOURCE_STATE_DEPTH_READ),
        FLAG_NAME(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        FLAG_NAME(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        FLAG_NAME(D3D12_RESOURCE_STATE_STREAM_OUT),
        FLAG_NAME(D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT),
        FLAG_NAME(D3D12_RESOURCE_STATE_COPY_DEST),
        FLAG_NAME(D3D12_RESOURCE_STATE_COPY_SOURCE),
        FLAG_NAME(D3D12_RESOURCE_STATE_RESOLVE_DEST),
        FLAG_NAME(D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
        FLAG_NAME(D3D12_RESOURCE_STATE_PRESENT),
        FLAG_NAME(D3D12_RESOURCE_STATE_PREDICATION),
        FLAG_NAME(D3D12_RESOURCE_STATE_VIDEO_DECODE_READ),
        FLAG_NAME(D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE),
        FLAG_NAME(D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ),
        FLAG_NAME(D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE),
        FLAG_NAME(D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ),
        FLAG_NAME(D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE),
    };

    sm::SmallVector<sm::StringView, std::size(kStates)> parts;
    for (auto [bit, name] : kStates) {
        if (states & bit) {
            parts.push_back(name);
        }
    }

    if (parts.empty()) {
        return "D3D12_RESOURCE_STATE_COMMON";
    }

    return fmt::format("D3D12_RESOURCE_STATE_{}", fmt::join(parts, "|"));
}
