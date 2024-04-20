// SPDX-License-Identifier: LGPL-3.0-only

#pragma once

#include "core/compiler.h"

CT_BEGIN_API

typedef struct arena_t arena_t;

/// @defgroup global_memory Global memory allocation
/// @ingroup runtime
/// @brief Default global memory allocator
/// this is an intermediate layer to help with transitioning to pure arena allocation
/// its recommended to avoid this for new code and to remove it from existing code
/// @{

/// @brief get the default allocator
///
/// @return the default allocator
arena_t *get_default_arena(void);

/// @} // global_memory

CT_END_API
