#pragma once

#include "threads/topology.hpp"

#include "core/library.hpp"

#define DLSYM(lib, name) lib.fn<decltype(&(name))>(#name)

namespace sm::threads::detail {
    using FnGetSystemCpuSetInformation = decltype(&GetSystemCpuSetInformation);
    using FnGetLogicalProcessorInformationEx = decltype(&GetLogicalProcessorInformationEx);

    struct DataBuffer {
        std::unique_ptr<uint8_t[]> data;
        DWORD size;
    };

    struct CpuInfoLibrary {
        sm::Library library;
        FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation;
        FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx;

        static CpuInfoLibrary load();
    };

    DataBuffer readSystemCpuSetInformation(FnGetSystemCpuSetInformation fn);
    DataBuffer readLogicalProcessorInformationEx(FnGetLogicalProcessorInformationEx fn, LOGICAL_PROCESSOR_RELATIONSHIP relation);
}
