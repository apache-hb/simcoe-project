#pragma once

// IWYU pragma: begin_exports

// tomlplusplus has a bug in its build script
// need to work around it here
#ifdef TOML_HEADER_ONLY
#   undef TOML_HEADER_ONLY
#endif

#define TOML_HEADER_ONLY 1
#define TOML_EXCEPTIONS 0

#include <toml++/toml.hpp>
#include <fmtlib/format.h>

#include <ranges>
#include <string_view>
#include <atomic>
#include <mutex>
#include <span>
#include <unordered_map>

#include "core/adt/bitset.hpp"
#include "core/core.hpp"

// IWYU pragma: end_exports
