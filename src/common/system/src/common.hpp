#pragma once

#include "core/win32.hpp" // IWYU pragma: export

extern HINSTANCE gInstance;
extern LPTSTR gWindowClass;

void initStorage();

#define SM_ASSERT_WIN32(expr)                                  \
    do {                                                       \
        if (auto result = (expr); !result) {                   \
            sm::system::assertLastError(CT_SOURCE_CURRENT, #expr); \
        }                                                      \
    } while (0)
