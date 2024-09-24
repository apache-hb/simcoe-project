#include "threads/threads.hpp"

#include "common.hpp"

#include "core/memory/unique.hpp"

#include "system/system.hpp"

using namespace sm;
using namespace sm::threads;

using CpuInfoLibrary = threads::detail::CpuInfoLibrary;

template<>
struct fmt::formatter<PROCESSOR_CACHE_TYPE> {
    constexpr auto parse(format_parse_context& ctx) const {
        return ctx.begin();
    }

    constexpr auto format(PROCESSOR_CACHE_TYPE type, format_context& ctx) const {
        switch (type) {
        case CacheUnified: return fmt::format_to(ctx.out(), "Unified");
        case CacheInstruction: return fmt::format_to(ctx.out(), "Instruction");
        case CacheData: return fmt::format_to(ctx.out(), "Data");
        case CacheTrace: return fmt::format_to(ctx.out(), "Trace");
        default: return fmt::format_to(ctx.out(), "Unknown");
        }
    }
};

CpuInfoLibrary CpuInfoLibrary::load() {
    sm::Library kernel32{"kernel32.dll"};
    auto pfnGetSystemCpuSetInformation = DLSYM(kernel32, GetSystemCpuSetInformation);
    auto pfnGetLogicalProcessorInformationEx = DLSYM(kernel32, GetLogicalProcessorInformationEx);

    return CpuInfoLibrary {
        std::move(kernel32),
        pfnGetSystemCpuSetInformation,
        pfnGetLogicalProcessorInformationEx
    };
}

namespace {
LOG_CATEGORY_IMPL(gThreadLog, "Threads");

threads::CpuGeometry gCpuGeometry;

#if 0
constexpr CacheType getCacheType(PROCESSOR_CACHE_TYPE type) noexcept {
    switch (type) {
    case CacheUnified: return CacheType::eUnified;
    case CacheInstruction: return CacheType::eInstruction;
    case CacheData: return CacheType::eData;
    case CacheTrace: return CacheType::eTrace;
    default: return CacheType::eUnknown;
    }
}
#endif

template <typename T>
T *advance(T *ptr, size_t bytes) noexcept {
    return reinterpret_cast<T *>(reinterpret_cast<std::byte *>(ptr) + bytes);
}

struct ProcessorInfoExIterator {
    ProcessorInfoExIterator(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD remaining) noexcept
        : mBuffer(buffer)
        , mRemaining(remaining)
    { }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator++() noexcept {
        CTASSERT(mRemaining > 0);

        mRemaining -= mBuffer->Size;
        if (mRemaining > 0) {
            mBuffer = advance(mBuffer, mBuffer->Size);
        }

        return mBuffer;
    }

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
    ProcessorInfoEx(LOGICAL_PROCESSOR_RELATIONSHIP relation, detail::FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx) noexcept {
        if (pfnGetLogicalProcessorInformationEx(relation, nullptr, &mSize)) {
            gThreadLog.warn("GetLogicalProcessorInformationEx{{1}} failed with error {}", sys::getLastError());
            return;
        }

        if (OsError err = sys::getLastError(); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
            gThreadLog.warn("GetLogicalProcessorInformationEx{{2}} failed with error {}", err);
            return;
        }

        mMemory = sm::UniquePtr<std::byte[]>(mSize);
        if (!pfnGetLogicalProcessorInformationEx(relation, getBuffer(), &mSize)) {
            gThreadLog.warn("GetLogicalProcessorInformationEx{{3}} failed with error {}", sys::getLastError());
            return;
        }

        mRemaining = mSize;
    }

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

    sm::UniquePtr<std::byte[]> mMemory;
    DWORD mSize = 0;

    DWORD mRemaining = 0;
};

struct ProcessorInfoIterator {
    ProcessorInfoIterator(SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer, DWORD count) noexcept
        : mBuffer(buffer)
        , mCount(count)
    { }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *operator++() noexcept {
        CTASSERT(mCount > 0);

        mCount -= 1;
        if (mCount > 0) {
            mBuffer += 1;
        }

        return mBuffer;
    }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *operator*() const noexcept {
        return mBuffer;
    }

    bool operator==(const ProcessorInfoIterator &other) const noexcept {
        return mCount == other.mCount;
    }

private:
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *mBuffer;
    DWORD mCount;
};

struct ProcessorInfo {
    ProcessorInfo(detail::FnGetLogicalProcessorInformation pfnGetLogicalProcessorInformation) {
        if (pfnGetLogicalProcessorInformation(nullptr, &mSize)) {
            gThreadLog.warn("GetLogicalProcessorInformation{{1}} failed with error {}", sys::getLastError());
            return;
        }

        if (OsError err = sys::getLastError(); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
            gThreadLog.warn("GetLogicalProcessorInformation{{2}} failed with error {}", err);
            return;
        }

        mMemory = sm::UniquePtr<std::byte[]>(mSize);

        if (!pfnGetLogicalProcessorInformation(getBuffer(), &mSize)) {
            gThreadLog.warn("GetLogicalProcessorInformation{{3}} failed with error {}", sys::getLastError());
            return;
        }

        if (mSize % sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) != 0) {
            gThreadLog.warn("GetLogicalProcessorInformation{{4}} returned invalid size {}. not a multiple of {}", mSize, sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
            return;
        }

        mCount = mSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    }

    ProcessorInfoIterator begin() const noexcept {
        return ProcessorInfoIterator(getBuffer(), mCount);
    }

    ProcessorInfoIterator end() const noexcept {
        return ProcessorInfoIterator(getBuffer(), 0);
    }

private:
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *getBuffer() const noexcept {
        return reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(mMemory.get());
    }

    sm::UniquePtr<std::byte[]> mMemory;
    DWORD mSize = 0;
    DWORD mCount = 0;
};

// GetSystemCpuSetInformation provides information about cpu speeds
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
    CpuSetInfo(detail::FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation) noexcept {
        HANDLE process = GetCurrentProcess();
        if (pfnGetSystemCpuSetInformation(nullptr, 0, &mSize, process, 0)) {
            gThreadLog.warn("GetSystemCpuSetInformation{{1}} failed with error {}", sys::getLastError());
            return;
        }

        if (OsError err = sys::getLastError(); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
            gThreadLog.warn("GetSystemCpuSetInformation{{2}} failed with error {}", err);
            return;
        }

        mMemory = sm::UniquePtr<std::byte[]>(mSize);
        mInfo = reinterpret_cast<SYSTEM_CPU_SET_INFORMATION *>(mMemory.get());
        if (!pfnGetSystemCpuSetInformation(mInfo, mSize, &mSize, process, 0)) {
            gThreadLog.warn("GetSystemCpuSetInformation{{3}} failed with error {}", sys::getLastError());
            return;
        }

        mRemaining = mSize;
    }

    CpuSetIterator begin() const noexcept {
        return CpuSetIterator(mInfo, mRemaining);
    }

    CpuSetIterator end() const noexcept {
        return CpuSetIterator(mInfo, 0);
    }

private:
    sm::UniquePtr<std::byte[]> mMemory;
    SYSTEM_CPU_SET_INFORMATION *mInfo = nullptr;
    ULONG mSize = 0;

    ULONG mRemaining = 0;
};

struct ProcessorInfoLayout {
    void addCoreInfo(const PROCESSOR_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Core: {:064b} {} {}", info.Flags, info.EfficiencyClass, info.GroupCount);
        for (WORD i = 0; i < info.GroupCount; i++) {
            fmt::println(stderr, "  Core Group {}: {:064b}", info.GroupMask[i].Group, info.GroupMask[i].Mask);
        }
    }

    void addCacheInfo(const CACHE_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Cache: level={} associativity={} lineSize={} cacheSize={} type={} groups={}",
            info.Level, info.Associativity, info.LineSize, info.CacheSize, info.Type, info.GroupCount);

        for (WORD i = 0; i < info.GroupCount; i++) {
            fmt::println(stderr, "  Cache Group {}: {:064b}", info.GroupMasks[i].Group, info.GroupMasks[i].Mask);
        }
    }

    void addNumaInfo(const NUMA_NODE_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Numa: node={} groups={}", info.NodeNumber, info.GroupCount);
        for (WORD i = 0; i < info.GroupCount; i++) {
            fmt::println(stderr, "  Numa Group {}: {:064b}", info.GroupMasks[i].Group, info.GroupMasks[i].Mask);
        }
    }

    void addPackageInfo(const PROCESSOR_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Package: {:064b} {} {}", info.Flags, info.EfficiencyClass, info.GroupCount);
        for (WORD i = 0; i < info.GroupCount; i++) {
            fmt::println(stderr, "  Package Group {}: {:064b}", info.GroupMask[i].Group, info.GroupMask[i].Mask);
        }
    }

    void addGroupInfo(const GROUP_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Group: {:064b} {}", info.MaximumGroupCount, info.ActiveGroupCount);
        for (WORD i = 0; i < info.ActiveGroupCount; i++) {
            fmt::println(stderr, "  Group {}: {:064b}", info.GroupInfo[i].MaximumProcessorCount, info.GroupInfo[i].ActiveProcessorCount);
        }
    }

    void addDieInfo(const PROCESSOR_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Die: {:064b} {} {}", info.Flags, info.EfficiencyClass, info.GroupCount);
        for (WORD i = 0; i < info.GroupCount; i++) {
            fmt::println(stderr, "  Die Group {}: {:064b}", info.GroupMask[i].Group, info.GroupMask[i].Mask);
        }
    }

    void addModuleInfo(const PROCESSOR_RELATIONSHIP &info) noexcept {
        fmt::println(stderr, "Module: {:064b} {} {}", info.Flags, info.EfficiencyClass, info.GroupCount);
        for (WORD i = 0; i < info.GroupCount; i++) {
            fmt::println(stderr, "  Module Group {}: {:064b}", info.GroupMask[i].Group, info.GroupMask[i].Mask);
        }
    }

    void addCpuSetInfo(const SYSTEM_CPU_SET_INFORMATION &info) noexcept {
        const auto& cpuSet = info.CpuSet;
        fmt::println(stderr,
            "CpuSet: id={} group={} logicalProcessorIndex={}, "
            "coreIndex={}, lastLevelCacheIndex={}, numaNodeIndex={}, efficiencyClass={},",
            cpuSet.Id, cpuSet.Group, cpuSet.LogicalProcessorIndex, cpuSet.CoreIndex, cpuSet.LastLevelCacheIndex, cpuSet.NumaNodeIndex, cpuSet.EfficiencyClass);

        fmt::println(stderr, "  Parked={}, Allocated={}, AllocatedToTargetProcess={}, RealTime={}", cpuSet.Parked, cpuSet.Allocated, cpuSet.AllocatedToTargetProcess, cpuSet.RealTime);
        fmt::println(stderr, "  SchedulingClass={}, AllocationTag={}", cpuSet.SchedulingClass, cpuSet.AllocationTag);
    }
};
}

CpuGeometry detail::buildCpuGeometry(const CpuInfoLibrary& library) noexcept {
    ProcessorInfoLayout processorInfoLayout{};

    if (library.pfnGetLogicalProcessorInformationEx != nullptr) {
        ProcessorInfoEx processorInfoEx{RelationAll, library.pfnGetLogicalProcessorInformationEx};

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorCore)
                processorInfoLayout.addCoreInfo(relation->Processor);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationCache)
                processorInfoLayout.addCacheInfo(relation->Cache);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationNumaNode)
                processorInfoLayout.addNumaInfo(relation->NumaNode);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorPackage)
                processorInfoLayout.addPackageInfo(relation->Processor);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationGroup)
                processorInfoLayout.addGroupInfo(relation->Group);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorDie)
                processorInfoLayout.addDieInfo(relation->Processor);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorModule)
                processorInfoLayout.addModuleInfo(relation->Processor);
        }
    } else {
        // LOG_WARN(ThreadLog, "No processor information available");
    }

    if (library.pfnGetSystemCpuSetInformation != nullptr) {
        CpuSetInfo cpusetInfo{library.pfnGetSystemCpuSetInformation};

        for (auto info : cpusetInfo) {
            if (info->Type == CpuSetInformation)
                processorInfoLayout.addCpuSetInfo(*info);
        }
    }

    // print cpu geometry

    return CpuGeometry { };
}

const CpuGeometry& threads::getCpuGeometry() noexcept {
    return gCpuGeometry;
}

void threads::init() noexcept {
    CpuInfoLibrary library = CpuInfoLibrary::load();
    gCpuGeometry = detail::buildCpuGeometry(library);
}
