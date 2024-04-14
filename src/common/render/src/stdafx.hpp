#pragma once

// IWYU pragma: begin_exports

#define NOMINMAX

#include <directx/d3d12.h>
#include <directx/d3dx12.h>
#include <dstorage.h>

#include "core/error.hpp"
#include "core/arena.hpp"
#include "os/core.h"

#include "core/vector.hpp"
#include "core/format.hpp"
#include "core/stack.hpp"
#include "core/map.hpp"

#include "logs/logs.hpp"

#include "fmt/std.h"

#include "math/math.hpp"
#include "math/format.hpp"
#include "math/hash.hpp"
#include "math/colour.hpp"

#include "tracy/Tracy.hpp"
#include "tracy/TracyD3D12.hpp"

#include "render/vendor/microsoft/pix.hpp"

#include "config/config.hpp"

// IWYU pragma: end_exports
