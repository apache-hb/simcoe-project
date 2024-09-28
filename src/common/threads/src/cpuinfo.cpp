#include "cpuinfo.hpp"

using namespace sm::threads::detail;

template <typename T>
T *advance(T *ptr, size_t bytes) noexcept {
    return reinterpret_cast<T *>(reinterpret_cast<std::byte *>(ptr) + bytes);
}

SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *ProcessorInfoExIterator::operator++() noexcept {
    CTASSERT(mRemaining > 0);

    mRemaining -= mBuffer->Size;
    if (mRemaining > 0) {
        mBuffer = advance(mBuffer, mBuffer->Size);
    }

    return mBuffer;
}

std::optional<ProcessorInfoEx> ProcessorInfoEx::create(LOGICAL_PROCESSOR_RELATIONSHIP relation, detail::FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx) noexcept {
    if (pfnGetLogicalProcessorInformationEx == nullptr) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx not available");
        return std::nullopt;
    }

    DWORD size = 0;
    if (pfnGetLogicalProcessorInformationEx(relation, nullptr, &size)) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", OsError(GetLastError()));
        return std::nullopt;
    }

    if (OsError err = OsError(GetLastError()); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", err);
        return std::nullopt;
    }

    auto memory = sm::UniquePtr<std::byte[]>(size);
    if (!pfnGetLogicalProcessorInformationEx(relation, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)memory.get(), &size)) {
        LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", OsError(GetLastError()));
        return std::nullopt;
    }

    return ProcessorInfoEx{std::move(memory), size};
}

ProcessorInfoEx ProcessorInfoEx::fromMemory(sm::UniquePtr<std::byte[]> memory, DWORD size) noexcept {
    return ProcessorInfoEx{std::move(memory), size};
}
