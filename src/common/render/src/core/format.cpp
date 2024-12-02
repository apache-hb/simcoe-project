#include "stdafx.hpp"

#include "render/base/format.hpp"
#include "render/next/render.hpp"

#include "core/adt/small_vector.hpp"

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

std::string sm::render::next::toString(DebugFlags flags) {
    std::vector<std::string_view> parts;

    static constexpr std::pair<DebugFlags, std::string_view> kFlags[] = {
        { DebugFlags::eDeviceDebugLayer, "DeviceDebugLayer" },
        { DebugFlags::eFactoryDebug, "FactoryDebug" },
        { DebugFlags::eDeviceRemovedInfo, "DeviceRemovedInfo" },
        { DebugFlags::eInfoQueue, "InfoQueue" },
        { DebugFlags::eAutoName, "AutoName" },
        { DebugFlags::eWarpAdapter, "WarpAdapter" },
        { DebugFlags::eGpuValidation, "GpuValidation" },
        { DebugFlags::eDirectStorageDebug, "DirectStorageDebug" },
        { DebugFlags::eDirectStorageBreak, "DirectStorageBreak" },
        { DebugFlags::eDirectStorageNames, "DirectStorageNames" },
        { DebugFlags::eWinPixEventRuntime, "WinPixEventRuntime" },
    };

    for (auto [flag, name] : kFlags) {
        if (bool(flags & flag)) {
            parts.push_back(name);
        }
    }

    return fmt::format("{}", fmt::join(parts, " | "));
}

std::string_view sm::render::next::toString(FeatureLevel level) {
    switch (level) {
    case FeatureLevel::eLevel_11_0: return "11.0";
    case FeatureLevel::eLevel_11_1: return "11.1";
    case FeatureLevel::eLevel_12_0: return "12.0";
    case FeatureLevel::eLevel_12_1: return "12.1";
    case FeatureLevel::eLevel_12_2: return "12.2";
    }

    return "Unknown";
}


std::string_view sm::render::next::toString(AdapterPreference pref) {
    switch (pref) {
    case AdapterPreference::eDefault: return "any";
    case AdapterPreference::eMinimumPower: return "low power";
    case AdapterPreference::eHighPerformance: return "high performance";
    }

    return "Unknown";
}