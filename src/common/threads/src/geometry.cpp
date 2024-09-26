#include "threads/threads.hpp"

#include "common.hpp"

#include "config/config.hpp"

#include "core/memory/unique.hpp"

#include "system/system.hpp"

using namespace sm;

namespace detail = sm::threads::detail;

using CpuInfoLibrary = threads::detail::CpuInfoLibrary;

static sm::opt<bool> gUsePinning {
    name = "pinning",
    desc = "Use core pinning",
    init = true
};

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
    static std::optional<ProcessorInfoEx> create(LOGICAL_PROCESSOR_RELATIONSHIP relation, detail::FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx) noexcept {
        if (pfnGetLogicalProcessorInformationEx == nullptr) {
            LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx not available");
            return std::nullopt;
        }

        DWORD size = 0;
        if (pfnGetLogicalProcessorInformationEx(relation, nullptr, &size)) {
            LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", sys::getLastError());
            return std::nullopt;
        }

        if (OsError err = sys::getLastError(); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
            LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", err);
            return std::nullopt;
        }

        auto memory = sm::UniquePtr<std::byte[]>(size);
        if (!pfnGetLogicalProcessorInformationEx(relation, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)memory.get(), &size)) {
            LOG_WARN(ThreadLog, "GetLogicalProcessorInformationEx failed with error {}", sys::getLastError());
            return std::nullopt;
        }

        return ProcessorInfoEx{std::move(memory), size};
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

    ProcessorInfoEx(sm::UniquePtr<std::byte[]> memory, DWORD size) noexcept
        : mMemory(std::move(memory))
        , mRemaining(size)
    { }

    sm::UniquePtr<std::byte[]> mMemory;

    DWORD mRemaining = 0;
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
    static std::optional<CpuSetInfo> create(detail::FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation) noexcept {
        if (pfnGetSystemCpuSetInformation == nullptr) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation not available");
            return std::nullopt;
        }

        HANDLE process = GetCurrentProcess();
        ULONG size = 0;
        if (pfnGetSystemCpuSetInformation(nullptr, 0, &size, process, 0)) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", sys::getLastError());
            return std::nullopt;
        }

        if (OsError err = sys::getLastError(); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", err);
            return std::nullopt;
        }

        auto memory = sm::UniquePtr<std::byte[]>(size);
        if (!pfnGetSystemCpuSetInformation((PSYSTEM_CPU_SET_INFORMATION)memory.get(), size, &size, process, 0)) {
            LOG_WARN(ThreadLog, "GetSystemCpuSetInformation failed with error {}", sys::getLastError());
            return std::nullopt;
        }

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

struct ProcessorLayout {
    std::vector<threads::LogicalCore> logicalCores;
    std::vector<threads::ProcessorCore> processorCores;
    std::vector<threads::Cache> caches;
    std::vector<threads::ProcessorPackage> packages;
    std::vector<threads::ProcessorDie> dies;
    std::vector<threads::NumaNode> numaNodes;
    std::vector<threads::ProcessorModule> modules;

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

threads::CpuGeometry detail::buildCpuGeometry(const CpuInfoLibrary& library) noexcept {
#if 0
    ProcessorLayout processorLayout{};

    if (auto maybeCpuSetInfo = CpuSetInfo::create(library.pfnGetSystemCpuSetInformation)) {
        CpuSetInfo cpusetInfo = std::move(*maybeCpuSetInfo);

        for (auto info : cpusetInfo) {
            if (info->Type == CpuSetInformation)
                processorLayout.addCpuSetInfo(*info);
        }
    }

    if (auto maybeProcessorInfoEx = ProcessorInfoEx::create(RelationAll, library.pfnGetLogicalProcessorInformationEx)) {
        ProcessorInfoEx processorInfoEx = std::move(*maybeProcessorInfoEx);

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorCore)
                processorLayout.addCoreInfo(relation->Processor);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationCache)
                processorLayout.addCacheInfo(relation->Cache);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationNumaNode)
                processorLayout.addNumaInfo(relation->NumaNode);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorPackage)
                processorLayout.addPackageInfo(relation->Processor);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationGroup)
                processorLayout.addGroupInfo(relation->Group);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorDie)
                processorLayout.addDieInfo(relation->Processor);
        }

        for (auto relation : processorInfoEx) {
            if (relation->Relationship == RelationProcessorModule)
                processorLayout.addModuleInfo(relation->Processor);
        }
    } else {
        LOG_WARN(ThreadLog, "No processor information available");

        return CpuGeometry { };
    }

    // print cpu geometry
#endif
    return CpuGeometry { };
}

const threads::CpuGeometry& threads::getCpuGeometry() noexcept {
    return gCpuGeometry;
}

void threads::init() noexcept {
    CpuInfoLibrary library = CpuInfoLibrary::load();
    gCpuGeometry = detail::buildCpuGeometry(library);
}
