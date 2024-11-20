#pragma once

#include "base/panic.h"

#define FMT_DISABLE_PRINT 1

#if CTU_ASSERTS
#   define FMT_ASSERT(expr, msg) CTASSERTM(expr, msg)
#else
#   define FMT_ASSERT(expr, msg) (void)(expr)
#endif

#define FMT_STATIC_THOUSANDS_SEPARATOR '\''

#ifdef __GNUC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdangling-reference"
#endif

#include "fmt/format.h" // IWYU pragma: export

#ifdef __GNUC__
#   pragma GCC diagnostic pop
#endif
