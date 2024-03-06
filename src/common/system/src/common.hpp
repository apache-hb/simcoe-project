#pragma once

#include "core/win32.hpp" // IWYU pragma: export

extern HINSTANCE gInstance;
extern LPTSTR gWindowClass;
extern size_t gTimerFrequency;

size_t query_timer();
