#pragma once

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
}
