#pragma once

#include "core/compiler.h"
#include "core/source_info.h"
#include "render/result.hpp" // IWYU pragma: export

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (auto result = (expr); FAILED(result)) {        \
            ::assert_hresult(CT_SOURCE_CURRENT, #expr, result); \
        }                                                  \
    } while (0)

CT_LOCAL CT_NORETURN assert_hresult(source_info_t source, const char *expr, HRESULT hr);
CT_LOCAL char *hresult_string(HRESULT hr);
