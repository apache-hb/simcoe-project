#pragma once

#include "base/panic.h"

#define FMT_THROW(x) CTASSERTM(false, (x).what())

#include "fmt/format.h" // IWYU pragma: export
