#include "stdafx.hpp"

#include "core/core.hpp"

extern "C" {
    extern char etext[];
    extern char edata[];
    extern char end[];
}

bool sm::isStaticStorageImpl(const void *ptr, [[maybe_unused]] bool fallback) noexcept {
    return ptr >= etext && ptr < end;
}
