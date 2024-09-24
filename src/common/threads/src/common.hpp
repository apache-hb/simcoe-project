#pragma once

#include "threads/threads.hpp"

#include "core/library.hpp"
#include "logs/structured/message.hpp"

#define DLSYM(lib, name) lib.fn<decltype(&(name))>(#name)

LOG_MESSAGE_CATEGORY(ThreadLog, "Threads");

namespace sm::threads::detail {
    using FnGetSystemCpuSetInformation = decltype(&GetSystemCpuSetInformation);
    using FnGetLogicalProcessorInformationEx = decltype(&GetLogicalProcessorInformationEx);

    struct CpuInfoLibrary {
        sm::Library library;
        FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation;
        FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx;

        static CpuInfoLibrary load();
    };

    CpuGeometry buildCpuGeometry(const CpuInfoLibrary& library) noexcept;
}
