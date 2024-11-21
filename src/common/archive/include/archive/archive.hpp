#pragma once

#if _WIN32
#include "core/win32.hpp"
#endif

#include "archive.dao.hpp"

#if _WIN32
namespace sm::archive {
    WINDOWPLACEMENT toWindowPlacement(const sm::dao::archive::WindowPlacement &placement) noexcept;
    sm::dao::archive::WindowPlacement fromWindowPlacement(const WINDOWPLACEMENT &placement) noexcept;
}
#endif