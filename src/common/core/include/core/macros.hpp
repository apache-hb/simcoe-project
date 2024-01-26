#pragma once

#include "core/macros.h" // IWYU pragma: export
#include "core/compiler.h"

#define SM_UNUSED [[maybe_unused]]

#define SM_NOCOPY(CLS) \
    CLS(const CLS&) = delete; \
    CLS& operator=(const CLS&) = delete;

#define SM_NOMOVE(CLS) \
    CLS(CLS&&) = delete; \
    CLS& operator=(CLS&&) = delete;
