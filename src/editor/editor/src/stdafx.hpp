#pragma once

// IWYU pragma: begin_exports

#define NOMINMAX
#include <directx/d3d12.h>
#include "DirectXTex/DirectXTex.h"

#include "backtrace/backtrace.h"
#include "base/panic.h"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "io/io.h"

#include "core/memory.h"
#include "core/error.hpp"
#include "core/format.hpp"
#include "core/macros.h"
#include "core/span.hpp"
#include "core/string.hpp"
#include "core/units.hpp"
#include "core/array.hpp"

#include "fmt/ranges.h"

#include "imgui/imgui.h"
#include "imgui/misc/imgui_stdlib.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"
#include "implot.h"
#include "ImGuizmo.h"
#include "imfilebrowser.h"
#include "imgui_memory_editor.h"

#include "logs/logs.hpp"

#include "math/math.hpp"
#include "math/format.hpp"
#include "math/hash.hpp"
#include "math/colour.hpp"

#include "render/vendor/microsoft/pix.hpp"

#include "fastgltf/core.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/tools.hpp"

#include "stb/stb_image.h"

// IWYU pragma: end_exports
