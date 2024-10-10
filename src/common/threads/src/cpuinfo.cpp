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

std::optional<ProcessorInfoEx> ProcessorInfoEx::create(LOGICAL_PROCESSOR_RELATIONSHIP relation, detail::FnGetLogicalProcessorInformationEx fn) noexcept {
    auto memory = detail::readLogicalProcessorInformationEx(fn, relation);
    if (memory.data == nullptr)
        return std::nullopt;

    return ProcessorInfoEx::fromMemory(std::move(memory.data), memory.size);
}

ProcessorInfoEx ProcessorInfoEx::fromMemory(std::unique_ptr<uint8_t[]> memory, DWORD size) noexcept {
    return ProcessorInfoEx{std::move(memory), size};
}
