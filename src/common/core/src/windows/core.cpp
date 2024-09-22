#include "stdafx.hpp"

#include "core/core.hpp"

#include <Psapi.h>

static MODULEINFO getModuleInfo() {
    MODULEINFO result = {0};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &result, sizeof(result));
    return result;
}

bool sm::isInStaticStorage(const void *ptr, [[maybe_unused]] bool fallback) noexcept {
    static MODULEINFO info = getModuleInfo();
    return ptr >= info.lpBaseOfDll && ptr < (char*)info.lpBaseOfDll + info.SizeOfImage;
}
