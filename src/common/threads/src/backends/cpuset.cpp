#include "common.hpp"
#include "cpuinfo.hpp"

namespace threads = sm::threads;
namespace detail = sm::threads::detail;

template <typename T>
T *advance(T *ptr, size_t bytes) noexcept {
    return reinterpret_cast<T *>(reinterpret_cast<std::byte *>(ptr) + bytes);
}

struct CpuSetIterator {
    CpuSetIterator(SYSTEM_CPU_SET_INFORMATION *pBuffer, ULONG remaining) noexcept
        : mBuffer(pBuffer)
        , mRemaining(remaining)
    { }

    SYSTEM_CPU_SET_INFORMATION *operator++() noexcept {
        CTASSERT(mRemaining > 0);

        mRemaining -= mBuffer->Size;
        if (mRemaining > 0) {
            mBuffer = advance(mBuffer, mBuffer->Size);
        }

        return mBuffer;
    }

    SYSTEM_CPU_SET_INFORMATION *operator*() const noexcept {
        return mBuffer;
    }

    bool operator==(const CpuSetIterator &other) const noexcept {
        return mRemaining == other.mRemaining;
    }

private:
    SYSTEM_CPU_SET_INFORMATION *mBuffer;
    ULONG mRemaining;
};

struct CpuSetInfo {
    static std::optional<CpuSetInfo> create(detail::FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation) noexcept {
        if (pfnGetSystemCpuSetInformation == nullptr) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation not available");
            return std::nullopt;
        }

        HANDLE process = GetCurrentProcess();
        ULONG size = 0;
        if (pfnGetSystemCpuSetInformation(nullptr, 0, &size, process, 0)) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", sm::OsError(GetLastError()));
            return std::nullopt;
        }

        if (sm::OsError err = GetLastError(); err != sm::OsError(ERROR_INSUFFICIENT_BUFFER)) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", err);
            return std::nullopt;
        }

        auto memory = sm::UniquePtr<std::byte[]>(size);
        if (!pfnGetSystemCpuSetInformation((PSYSTEM_CPU_SET_INFORMATION)memory.get(), size, &size, process, 0)) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", sm::OsError(GetLastError()));
            return std::nullopt;
        }

        return CpuSetInfo::fromMemory(std::move(memory), size);
    }

    static CpuSetInfo fromMemory(sm::UniquePtr<std::byte[]> memory, ULONG size) noexcept {
        return CpuSetInfo{std::move(memory), size};
    }

    CpuSetIterator begin() const noexcept {
        return CpuSetIterator(getBuffer(), mRemaining);
    }

    CpuSetIterator end() const noexcept {
        return CpuSetIterator(getBuffer(), 0);
    }

private:
    CpuSetInfo(sm::UniquePtr<std::byte[]> memory, ULONG size) noexcept
        : mMemory(std::move(memory))
        , mRemaining(size)
    { }

    SYSTEM_CPU_SET_INFORMATION *getBuffer() const noexcept {
        return reinterpret_cast<SYSTEM_CPU_SET_INFORMATION *>(mMemory.get());
    }

    sm::UniquePtr<std::byte[]> mMemory;

    ULONG mRemaining = 0;
};

class CpuSetScheduler final : public threads::IScheduler {
    std::vector<DWORD> mCpuSetIds;

    threads::ThreadHandle launchThread(void *param, LPTHREAD_START_ROUTINE start) override {
        DWORD id = 0;
        HANDLE thread = CreateThread(nullptr, 0, start, param, CREATE_SUSPENDED, &id);
        if (thread == nullptr) {
            auto error = sm::OsError(GetLastError());
            LOG_ERROR(ThreadLog, "Failed to create thread: {}", error);
            error.raise();
        }

        threads::ThreadHandle handle(thread, id);

        BOOL bResult = SetThreadSelectedCpuSets(thread, mCpuSetIds.data(), mCpuSetIds.size());
        if (!bResult) {
            auto error = sm::OsError(GetLastError());
            LOG_ERROR(ThreadLog, "Failed to set thread affinity: {}", error);
        }

        ResumeThread(thread);

        return handle;
    }

public:
    CpuSetScheduler(const std::vector<threads::LogicalCore>& threads) noexcept
        : mCpuSetIds(threads.size())
    {
        for (size_t i = 0; i < threads.size(); ++i) {
            mCpuSetIds[i] = threads[i].id;
        }
    }
};

class CpuSetGeometry final : public threads::ICpuGeometry {
    std::vector<SYSTEM_CPU_SET_INFORMATION> mCpuSets;

    void initCpuSets(const CpuSetInfo& info) {
        for (auto cpuSet : info) {
            // msdn says to ignore any types you don't explicitly handle
            // for forward compatibility
            if (cpuSet->Type != CpuSetInformation)
                continue;

            mCpuSets.push_back(*cpuSet);
        }

        std::sort(mCpuSets.begin(), mCpuSets.end(), [](const SYSTEM_CPU_SET_INFORMATION &lhs, const SYSTEM_CPU_SET_INFORMATION &rhs) {
            // sort first by group, then logicalProcessorIndex
            return std::tie(lhs.CpuSet.Group, lhs.CpuSet.LogicalProcessorIndex)
                 < std::tie(rhs.CpuSet.Group, rhs.CpuSet.LogicalProcessorIndex);
        });
    }

    void initLogicalCores() {
        mLogicalCores.reserve(mCpuSets.size());

        for (const auto &cpuSet : mCpuSets) {
            threads::LogicalCore core = {
                .id = cpuSet.CpuSet.Id,
                .group = cpuSet.CpuSet.Group,
                .coreIndex = cpuSet.CpuSet.CoreIndex,
                .threadIndex = cpuSet.CpuSet.LogicalProcessorIndex,
            };

            mLogicalCores.push_back(core);
        }
    }

    void initProcessorCores() {
        mProcessorCores.reserve(mCpuSets.size());

        for (const auto &cpuSet : mCpuSets) {
            threads::ProcessorCore core = {
                .schedule = cpuSet.CpuSet.SchedulingClass,
                .efficiency = cpuSet.CpuSet.EfficiencyClass,
            };

            mProcessorCores.push_back(core);
        }
    }

    void initCacheInfo(const detail::ProcessorInfoEx& layout) {
        for (const auto &relation : layout) {
            if (relation->Relationship != RelationCache)
                continue;

            const CACHE_RELATIONSHIP *info = &relation->Cache;

            threads::Cache cache = {
                .level = info->Level,
                .associativity = info->Associativity,
                .lineSize = info->LineSize,
                .size = info->CacheSize,
                .type = detail::getCacheType(info->Type),
            };

            mCaches.push_back(cache);
        }
    }

public:
    threads::IScheduler *newScheduler(std::vector<threads::LogicalCore> threads) override {
        return new CpuSetScheduler(threads);
    }

    CpuSetGeometry(const CpuSetInfo& info, const detail::ProcessorInfoEx& layout) {
        initCpuSets(info);
        initLogicalCores();
        initProcessorCores();
        initCacheInfo(layout);
        createCoreGroups();
    }
};

threads::ICpuGeometry *detail::newCpuSetGeometry(
    detail::FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx,
    detail::FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation
) {
    CTASSERT(pfnGetLogicalProcessorInformationEx != nullptr);
    CTASSERT(pfnGetSystemCpuSetInformation != nullptr);

    auto cpuSetInfo = CpuSetInfo::create(pfnGetSystemCpuSetInformation);
    if (!cpuSetInfo)
        return nullptr;

    auto layoutInfo = ProcessorInfoEx::create(RelationAll, pfnGetLogicalProcessorInformationEx);
    if (!layoutInfo)
        return nullptr;

    return new CpuSetGeometry(*cpuSetInfo, *layoutInfo);
}
