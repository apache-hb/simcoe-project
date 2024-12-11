#include "core/cpuid.hpp"

#include <cpuid.h>

using namespace sm;

void CpuId::asString(char dst[16]) const noexcept {
    memcpy(dst, ureg, sizeof(ureg));
}

CpuId CpuId::of(int leaf) noexcept {
    CpuId result{};
    __cpuid(leaf, result.eax, result.ebx, result.ecx, result.edx);
    return result;
}

CpuId CpuId::count(int leaf, int subleaf) noexcept {
    CpuId result{};
    __cpuid_count(leaf, subleaf, result.eax, result.ebx, result.ecx, result.edx);
    return result;
}

bool CpuId::getBrandString(char dst[kBrandStringSize]) noexcept {
    CpuId check = CpuId::of(0x80000000);
    if (check.eax < 0x80000004)
        return false;

    CpuId::of(0x80000002).asString(dst + 0);
    CpuId::of(0x80000003).asString(dst + 16);
    CpuId::of(0x80000004).asString(dst + 32);

    return true;
}
