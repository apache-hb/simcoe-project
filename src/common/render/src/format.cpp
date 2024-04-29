#include "stdafx.hpp"

#include "render/format.hpp"

#include <fmt/ranges.h>

sm::String sm::render::format(D3D12_RESOURCE_STATES states) {
    struct BitName {
        D3D12_RESOURCE_STATES state;
        const char *name;
    };

    static constexpr BitName kStates[] = {
        { D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, "vbo/cbuffer" },
        { D3D12_RESOURCE_STATE_INDEX_BUFFER, "ibo" },
        { D3D12_RESOURCE_STATE_RENDER_TARGET, "rt" },
        { D3D12_RESOURCE_STATE_UNORDERED_ACCESS, "unordered_access" },
        { D3D12_RESOURCE_STATE_DEPTH_WRITE, "depth_write" },
        { D3D12_RESOURCE_STATE_DEPTH_READ, "depth_read" },
        { D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, "non_pixel_shader_resource" },
        { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "pixel_shader_resource" },
        { D3D12_RESOURCE_STATE_STREAM_OUT, "stream_out" },
        { D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, "indirect" },
        { D3D12_RESOURCE_STATE_COPY_DEST, "copy_dst" },
        { D3D12_RESOURCE_STATE_COPY_SOURCE, "copy_src" },
        { D3D12_RESOURCE_STATE_VIDEO_DECODE_READ, "video_decode_read" },
        { D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE, "video_decode_write" },
        { D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ, "video_process_read" },
        { D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE, "video_process_write" },
        { D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ, "video_encode_read" },
        { D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE, "video_encode_write" }
    };

    sm::SmallVector<sm::StringView, std::size(kStates)> parts;
    for (auto [bit, name] : kStates) {
        if (states & bit) {
            parts.push_back(name);
        }
    }

    return fmt::format("state({})", fmt::join(parts, "|"));
}
