#include "common.hpp"

#include "core/arena.hpp"

#include "os/core.h"

char *hresult_string(HRESULT hr) {
    sm::IArena& arena = sm::get_pool(sm::Pool::eDebug);
    return os_error_string(hr, &arena);
}

CT_NORETURN
assert_hresult(source_info_t source, const char *expr, HRESULT hr) {
    ctu_panic(source, "hresult: %s %s", hresult_string(hr), expr);
}

char *sm::render::Result::to_string() const {
    return hresult_string(mValue);
}
