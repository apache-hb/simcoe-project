#pragma once

#include "base/panic.h"

#define FMT_DISABLE_PRINT 1
#define FMT_ASSERT(expr, msg) CTASSERTM(expr, msg)
#define FMT_THROW(x) CTASSERTM(false, (x).what())
#define FMT_EXCEPTIONS 0
#define FMT_STATIC_THOUSANDS_SEPARATOR '\''

#include "fmt/format.h" // IWYU pragma: export

#define SM_FORMAT 1
