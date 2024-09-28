#pragma once

#include "threads/threads.hpp"

#include "core/library.hpp"

#define DLSYM(lib, name) lib.fn<decltype(&(name))>(#name)

namespace sm::threads::detail {
    using FnGetSystemCpuSetInformation = decltype(&GetSystemCpuSetInformation);
    using FnGetLogicalProcessorInformationEx = decltype(&GetLogicalProcessorInformationEx);

    struct CpuInfoLibrary {
        sm::Library library;
        FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation;
        FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx;

        static CpuInfoLibrary load();
    };

    ICpuGeometry *buildCpuGeometry(const CpuInfoLibrary& library);

    ICpuGeometry *newCpuSetGeometry(
        FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx,
        FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation
    );

    ICpuGeometry *newProcessorGeometry(FnGetLogicalProcessorInformationEx pfn);
    ICpuGeometry *newDefaultGeometry();
}
