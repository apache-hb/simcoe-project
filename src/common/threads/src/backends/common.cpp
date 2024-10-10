#include "common.hpp"

namespace threads = sm::threads;
namespace detail = sm::threads::detail;

detail::DataBuffer detail::readSystemCpuSetInformation(FnGetSystemCpuSetInformation fn) {
    if (fn == nullptr) {
        LOG_WARN(ThreadLog, "GetSystemCpuSetInformation not available");
        return DataBuffer{};
    }

    HANDLE process = GetCurrentProcess();
    ULONG size = 0;
    if (fn(nullptr, 0, &size, process, 0)) {
        LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", sm::OsError(GetLastError()));
        return DataBuffer{};
    }

    if (sm::OsError err = GetLastError(); err != sm::OsError(ERROR_INSUFFICIENT_BUFFER)) {
        LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", err);
        return DataBuffer{};
    }

    auto memory = std::make_unique<uint8_t[]>(size);
    if (!fn((PSYSTEM_CPU_SET_INFORMATION)memory.get(), size, &size, process, 0)) {
        LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", sm::OsError(GetLastError()));
        return DataBuffer{};
    }

    return DataBuffer{ std::move(memory), size };
}

detail::DataBuffer detail::readLogicalProcessorInformationEx(FnGetLogicalProcessorInformationEx fn, LOGICAL_PROCESSOR_RELATIONSHIP relation) {
    if (fn == nullptr) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx not available");
        return DataBuffer{};
    }

    DWORD size = 0;
    if (fn(relation, nullptr, &size)) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", OsError(GetLastError()));
        return DataBuffer{};
    }

    if (OsError err = OsError(GetLastError()); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", err);
        return DataBuffer{};
    }

    auto memory = std::make_unique<uint8_t[]>(size);
    if (!fn(relation, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)memory.get(), &size)) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", OsError(GetLastError()));
        return DataBuffer{};
    }

    return DataBuffer{ std::move(memory), size };
}
