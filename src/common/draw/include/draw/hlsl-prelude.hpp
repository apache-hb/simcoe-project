#pragma once

#ifdef __HLSL_VERSION
#   define static_assert(x, ...)
#   define CXX(...)
#   define INTEROP_BEGIN(ns)
#   define INTEROP_END(ns)
#else
#   include "math/math.hpp" // IWYU pragma: export
#   define row_major
#   define column_major
#   define CXX(...) __VA_ARGS__
#   define INTEROP_BEGIN(ns) namespace ns { using namespace sm::math;
#   define INTEROP_END(ns) }
#endif

#define constexpr CXX(constexpr)
#define noexcept CXX(noexcept)
