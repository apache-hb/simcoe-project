#pragma once

#include "logs/logs.hpp"
#include "core/win32.hpp" // IWYU pragma: export

extern HINSTANCE gInstance;
extern LPTSTR gWindowClass;

LOG_CATEGORY(gSystemLog);
