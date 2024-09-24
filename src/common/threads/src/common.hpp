#pragma once

#include "threads/threads.hpp"

#include "core/library.hpp"

#define DLSYM(lib, name) lib.fn<decltype(&(name))>(#name)

namespace sm::threads::detail {
    using FnGetSystemCpuSetInformation = decltype(&GetSystemCpuSetInformation);
    using FnGetLogicalProcessorInformation = decltype(&GetLogicalProcessorInformation);
    using FnGetLogicalProcessorInformationEx = decltype(&GetLogicalProcessorInformationEx);

    struct CpuInfoLibrary {
        sm::Library library;
        FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation;
        FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx;

        static CpuInfoLibrary load();
    };

    CpuGeometry buildCpuGeometry(const CpuInfoLibrary& library) noexcept;
}
