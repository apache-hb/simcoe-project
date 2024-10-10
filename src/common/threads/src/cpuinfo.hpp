#pragma once

#include "backends/common.hpp"

namespace sm::threads::detail {
    constexpr CacheType getCacheType(PROCESSOR_CACHE_TYPE type) noexcept {
        switch (type) {
        case CacheUnified: return CacheType::eUnified;
        case CacheInstruction: return CacheType::eInstruction;
        case CacheData: return CacheType::eData;
        case CacheTrace: return CacheType::eTrace;
        default: return CacheType::eUnknown;
        }
    }

    struct ProcessorInfoExIterator {
        ProcessorInfoExIterator(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD remaining) noexcept
            : mBuffer(buffer)
            , mRemaining(remaining)
        { }

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator++() noexcept;

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator*() const noexcept {
            return mBuffer;
        }

        bool operator==(const ProcessorInfoExIterator &other) const noexcept {
            return mRemaining == other.mRemaining;
        }

    private:
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *mBuffer;
        DWORD mRemaining;
    };

    // GetLogicalProcessorInformationEx provides information about inter-core communication
    struct ProcessorInfoEx {
        static std::optional<ProcessorInfoEx> create(LOGICAL_PROCESSOR_RELATIONSHIP relation, detail::FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx) noexcept;

        static ProcessorInfoEx fromMemory(std::unique_ptr<uint8_t[]> memory, DWORD size) noexcept;

        ProcessorInfoExIterator begin() const noexcept {
            return ProcessorInfoExIterator(getBuffer(), mRemaining);
        }

        ProcessorInfoExIterator end() const noexcept {
            return ProcessorInfoExIterator(getBuffer(), 0);
        }

    private:
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *getBuffer() const noexcept {
            return reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(mMemory.get());
        }

        ProcessorInfoEx(std::unique_ptr<uint8_t[]> memory, DWORD size) noexcept
            : mMemory(std::move(memory))
            , mRemaining(size)
        { }

        std::unique_ptr<uint8_t[]> mMemory;

        DWORD mRemaining = 0;
    };
}
