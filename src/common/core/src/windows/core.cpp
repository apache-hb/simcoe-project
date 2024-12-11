#include "stdafx.hpp"

#include "core/core.hpp"

#include <Psapi.h>

static MODULEINFO getModuleInfo() noexcept {
    MODULEINFO result = {0};
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &result, sizeof(result));
    return result;
}

bool sm::isStaticStorageImpl(const void *ptr, [[maybe_unused]] bool fallback) noexcept {
    static const MODULEINFO sModuleInfo = getModuleInfo();
    return ptr >= sModuleInfo.lpBaseOfDll && ptr < (char*)sModuleInfo.lpBaseOfDll + sModuleInfo.SizeOfImage;
}
