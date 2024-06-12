#include "stdafx.hpp"

#include "core/core.hpp"

extern "C" {
    extern char etext[];
    extern char edata[];
    extern char end[];
}

bool sm::isInStaticStorage(const void *ptr) noexcept {
    return ptr >= etext && ptr < edata;
}
