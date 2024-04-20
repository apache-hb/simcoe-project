#pragma once

#include <simcoe_config.h>

#include "core/win32.hpp" // IWYU pragma: export
#include "core/compiler.h"

#if SMC_PIX_ENABLE
#   define USE_PIX 1
#endif

CT_CLANG_PRAGMA(clang diagnostic push)
CT_CLANG_PRAGMA(clang diagnostic ignored "-Wunused-but-set-variable")

#include "WinPixEventRuntime/pix3.h"

CT_CLANG_PRAGMA(clang diagnostic pop)
