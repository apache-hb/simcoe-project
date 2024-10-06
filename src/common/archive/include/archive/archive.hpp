#pragma once

#include "core/win32.hpp"

#include "archive.dao.hpp"

namespace sm::archive {
    WINDOWPLACEMENT toWindowPlacement(const sm::dao::archive::WindowPlacement &placement) noexcept;
    sm::dao::archive::WindowPlacement fromWindowPlacement(const WINDOWPLACEMENT &placement) noexcept;
}
