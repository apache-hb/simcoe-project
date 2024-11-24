#pragma once

#include <directx/d3d12.h>

#include <string>
#include <string_view>

#include "reflect/reflect.h"

#define REFLECT_ENUM_BITFLAGS_EX(ENUM) \
    REFLECT_ENUM_BITFLAGS(ENUM, __underlying_type(ENUM)) \
    constexpr inline ENUM operator|=(ENUM& a, ENUM b) { return a = a | b; } \
    constexpr inline ENUM operator&=(ENUM& a, ENUM b) { return a = a & b; } \
    constexpr inline ENUM operator^=(ENUM& a, ENUM b) { return a = a ^ b; }

namespace sm::render::next {
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

    REFLECT_ENUM_BITFLAGS_EX(DebugFlags)

    enum class FeatureLevel {
        eLevel_11_0 = D3D_FEATURE_LEVEL_11_0,
        eLevel_11_1 = D3D_FEATURE_LEVEL_11_1,
        eLevel_12_0 = D3D_FEATURE_LEVEL_12_0,
        eLevel_12_1 = D3D_FEATURE_LEVEL_12_1,
        eLevel_12_2 = D3D_FEATURE_LEVEL_12_2,
    };

    std::string toString(DebugFlags flags);
    std::string_view toString(FeatureLevel level);
}
