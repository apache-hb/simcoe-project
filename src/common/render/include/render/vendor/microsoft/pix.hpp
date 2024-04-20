#pragma once

#include <simcoe_config.h>

#include "core/win32.hpp" // IWYU pragma: export
#include "core/compiler.h"

#if SMC_PIX_ENABLE
#   define USE_PIX 1
#endif

#if defined(__clang__)
CT_PRAGMA(clang diagnostic push)
CT_PRAGMA(clang diagnostic ignored "-Wunused-but-set-variable")
#endif

#include "WinPixEventRuntime/pix3.h"

#if defined(__clang__)
CT_PRAGMA(clang diagnostic pop)
#endif
