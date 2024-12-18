#pragma once

#ifdef __HLSL_VERSION
#   undef static_assert
#else
#   undef row_major
#   undef column_major
#endif

#undef constexpr
#undef noexcept

#undef INTEROP_BEGIN
#undef INTEROP_END
#undef CXX
