#pragma once

#include <directx/d3d12.h>

#include "reflect/reflect.h"

#include "render.meta.hpp"

namespace sm::render::next {
    REFLECT_ENUM(DebugFlags)
    enum class DebugFlags {
        eNone = 0,
        eDeviceDebugLayer = (1 << 0),
        eFactoryDebug = (1 << 1),
        eDeviceRemovedInfo = (1 << 2),
        eInfoQueue = (1 << 3),
        eAutoName = (1 << 4),
        eWarpAdapter = (1 << 5),
        eGpuValidation = (1 << 6),
        eDirectStorageDebug = (1 << 7),
        eDirectStorageBreak = (1 << 8),
        eDirectStorageNames = (1 << 9),
        eWinPixEventRuntime = (1 << 10),
    };

    REFLECT_ENUM_BITFLAGS(DebugFlags, __underlying_type(DebugFlags))

    constexpr inline DebugFlags operator|=(DebugFlags& a, DebugFlags b) {
        return a = a | b;
    }

    constexpr inline DebugFlags operator&=(DebugFlags& a, DebugFlags b) {
        return a = a & b;
    }

    REFLECT_ENUM(FeatureLevel)
    enum class FeatureLevel {
        eLevel_11_0 = D3D_FEATURE_LEVEL_11_0,
        eLevel_11_1 = D3D_FEATURE_LEVEL_11_1,
        eLevel_12_0 = D3D_FEATURE_LEVEL_12_0,
        eLevel_12_1 = D3D_FEATURE_LEVEL_12_1,
        eLevel_12_2 = D3D_FEATURE_LEVEL_12_2,
    };
}
