#include "threads/threads.hpp"

#include "core/memory/unique.hpp"
#include "core/span.hpp"

#include "system/system.hpp"

using namespace sm;
using namespace sm::threads;

namespace {
LOG_CATEGORY_IMPL(gThreadLog, "threads");

threads::CpuGeometry gCpuGeometry;

using FnGetSystemCpuSetInformation = decltype(&GetSystemCpuSetInformation);
using FnGetLogicalProcessorInformationEx = decltype(&GetLogicalProcessorInformationEx);

constexpr CacheType getCacheType(PROCESSOR_CACHE_TYPE type) noexcept {
    switch (type) {
    case CacheUnified: return CacheType::eUnified;
    case CacheInstruction: return CacheType::eInstruction;
    case CacheData: return CacheType::eData;
    case CacheTrace: return CacheType::eTrace;
    default: return CacheType::eUnknown;
    }
}

struct PartialPackageInfo {
    sm::VectorBase<GROUP_AFFINITY> masks;
};

class GeometryBuilder {
    HMODULE mKernel32Handle;
    FnGetSystemCpuSetInformation mGetSystemCpuSetInformation = nullptr;
    FnGetLogicalProcessorInformationEx mGetLogicalProcessorInformationEx = nullptr;

    static FARPROC loadModuleEntry(HMODULE mod, const char *name, const char *pfn) noexcept {
        FARPROC proc = ::GetProcAddress(mod, name);
        if (proc == nullptr) {
            gThreadLog.warn("{} did not contain {}", pfn, name);
        }

        return proc;
    }

    template<typename T, typename I>
    static IndexRange<I> getMaskedRange(sm::Span<const T> span, GROUP_AFFINITY mask) noexcept {
        static_assert(std::is_base_of_v<ScheduleMask, T>, "T must be derived from ScheduleMask");

        const I first = [&] {
            // scan from the start to find the first item with a matching mask
            for (size_t i = 0; i < span.size(); ++i) {
                if (span[i].isContainedIn(mask)) {
                    return enum_cast<I>(i);
                }
            }

            SM_NEVER("no items found with a mask matching {}:{}", mask.Group, mask.Mask);
        }();

        const I last = [&] {
            // scan from the end to find the last item with a matching mask
            for (size_t i = span.size(); i > 0; --i) {
                if (span[i - 1].isContainedIn(mask)) {
                    return enum_cast<I>(i);
                }
            }

            SM_NEVER("no items found with a mask matching {}:{}", mask.Group, mask.Mask);
        }();

        return {first, last};
    }

public:
    GeometryBuilder() noexcept {
        mKernel32Handle = ::GetModuleHandleA("kernel32");
        if (mKernel32Handle == nullptr) {
            gThreadLog.warn("failed to load kernel32.dll {}", sys::getLastError());
            return;
        }

        mGetSystemCpuSetInformation = (FnGetSystemCpuSetInformation)(void*)loadModuleEntry(mKernel32Handle, "GetSystemCpuSetInformation", "kernel32.dll");
        mGetLogicalProcessorInformationEx = (FnGetLogicalProcessorInformationEx)(void*)loadModuleEntry(mKernel32Handle, "GetLogicalProcessorInformationEx", "kernel32.dll");
    }

    FnGetSystemCpuSetInformation getSystemCpuSetInformation() const noexcept {
        return mGetSystemCpuSetInformation;
    }

    FnGetLogicalProcessorInformationEx getLogicalProcessorInformationEx() const noexcept {
        return mGetLogicalProcessorInformationEx;
    }

    // TODO: figure out the max number of subcores for the cpu and do this all with arrays

    sm::VectorBase<HyperThread> threads;
    sm::VectorBase<Core> cores;
    sm::VectorBase<Group> groups;
    sm::VectorBase<Cache> caches;

    sm::VectorBase<PartialPackageInfo> pendingPackages;

    IndexRange<ThreadIndex> getMaskedThreadRange(GROUP_AFFINITY mask) const noexcept {
        return getMaskedRange<HyperThread, ThreadIndex>(threads, mask);
    }

    IndexRange<CoreIndex> getMaskedCoreRange(GROUP_AFFINITY mask) const noexcept {
        return getMaskedRange<Core, CoreIndex>(cores, mask);
    }

    // IndexRange<ChipletIndex> getMaskedChipletRange(GROUP_AFFINITY mask) const noexcept {
    //     return getMaskedRange<Chiplet, ChipletIndex>(chiplets, mask);
    // }

    threads::CpuGeometry build() noexcept {
        auto predicate = [](const ScheduleMask& lhs, const ScheduleMask& rhs) -> bool {
            // sort first by group and then by mask
            return (lhs.mask.Group > rhs.mask.Group) || ((lhs.mask.Group == rhs.mask.Group) && (lhs.mask.Mask > rhs.mask.Mask));
        };

        // first sort all acquired data by its affinity so we can find index ranges
        std::sort(threads.begin(), threads.end(), predicate);
        std::sort(cores.begin(), cores.end(), predicate);
        std::sort(groups.begin(), groups.end(), predicate);

        sm::VectorBase<Package> packages = [&] {
            sm::VectorBase<Package> result;

            return result;
        }();

        for (size_t i = 0; i < packages.size(); i++) {
            gThreadLog.info("package {}: {} threads, {} cores, {} group(s), {} caches",
                i, packages[i].threads.size(), packages[i].cores.size(),
                packages[i].groups.size(), packages[i].caches.size());
        }

        gThreadLog.info("cpu geometry: {} total package(s), {} total group(s), {} total cores, {} total threads",
            packages.size(), groups.size(), cores.size(),
            threads.size());

        return CpuGeometry {
            .threads = std::move(threads),
            .cores = std::move(cores),
            .caches = {},
            .groups = std::move(groups),
            .packages = std::move(packages),
        };
    }
};

template <typename T>
T *advance(T *ptr, size_t bytes) noexcept {
    return reinterpret_cast<T *>(reinterpret_cast<std::byte *>(ptr) + bytes);
}

struct ProcessorInfoIterator {
    ProcessorInfoIterator(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD remaining) noexcept
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

    bool operator==(const ProcessorInfoIterator &other) const noexcept {
        return mRemaining == other.mRemaining;
    }

private:
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *mBuffer;
    DWORD mRemaining;
};

// GetLogicalProcessorInformationEx provides information about inter-core communication
struct ProcessorInfo {
    ProcessorInfo(LOGICAL_PROCESSOR_RELATIONSHIP relation, const GeometryBuilder& builder) noexcept {
        initInfo(relation, builder.getLogicalProcessorInformationEx());
    }

    ProcessorInfoIterator begin() const noexcept {
        return ProcessorInfoIterator(mBuffer, mRemaining);
    }

    ProcessorInfoIterator end() const noexcept {
        return ProcessorInfoIterator(mBuffer, 0);
    }

private:
    void initInfo(LOGICAL_PROCESSOR_RELATIONSHIP relation, FnGetLogicalProcessorInformationEx pfnGetLogicalProcessorInformationEx) noexcept {
        if (pfnGetLogicalProcessorInformationEx(relation, nullptr, &mSize)) {
            gThreadLog.warn("GetLogicalProcessorInformationEx{{1}} failed with error {}", sys::getLastError());
            return;
        }

        if (OsError err = sys::getLastError(); err != OsError(ERROR_INSUFFICIENT_BUFFER)) {
            gThreadLog.warn("GetLogicalProcessorInformationEx{{2}} failed with error {}", err);
            return;
        }

        mMemory = sm::UniquePtr<std::byte[]>(mSize);
        mBuffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(mMemory.get());
        if (!pfnGetLogicalProcessorInformationEx(relation, mBuffer, &mSize)) {
            gThreadLog.warn("GetLogicalProcessorInformationEx{{3}} failed with error {}", sys::getLastError());
            return;
        }

        mRemaining = mSize;
    }

    sm::UniquePtr<std::byte[]> mMemory;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *mBuffer = nullptr;
    DWORD mSize = 0;

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
    CpuSetInfo(GeometryBuilder& builder) noexcept {
        initInfo(builder.getSystemCpuSetInformation());
    }

    CpuSetIterator begin() const noexcept {
        return CpuSetIterator(mInfo, mRemaining);
    }

    CpuSetIterator end() const noexcept {
        return CpuSetIterator(mInfo, 0);
    }

private:
    void initInfo(FnGetSystemCpuSetInformation pfnGetSystemCpuSetInformation) noexcept {
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

    sm::UniquePtr<std::byte[]> mMemory;
    SYSTEM_CPU_SET_INFORMATION *mInfo = nullptr;
    ULONG mSize = 0;

    ULONG mRemaining = 0;
};

class ProcessorInfoLayout {
    GeometryBuilder &mBuilder;

    uint mCacheId = 0;

public:
    ProcessorInfoLayout(GeometryBuilder& build) noexcept
        : mBuilder(build)
    { }

    static constexpr KAFFINITY kAffinityBits = std::numeric_limits<KAFFINITY>::digits;

    void addCoreInfo(const PROCESSOR_RELATIONSHIP &info) noexcept {
        for (DWORD i = 0; i < info.GroupCount; ++i) {
            GROUP_AFFINITY group = info.GroupMask[i];

            for (size_t bit = 0; bit < kAffinityBits; ++bit) {
                KAFFINITY mask = 1ull << bit;
                if (!(group.Mask & mask)) continue;

                GROUP_AFFINITY groupAffinity = {
                    .Mask = group.Mask & mask,
                    .Group = group.Group,
                };

                threads::HyperThread info = {
                    { .mask = groupAffinity },
                };

                mBuilder.threads.push_back(info);
            }
        }

        threads::Core core = {
            /*ScheduleMask=*/ { info.GroupMask[0] },
            /*threads=*/ { ThreadIndex::eInvalid, ThreadIndex::eInvalid },
            /*schedule =*/ 0,
            /*efficiency=*/ info.EfficiencyClass,
        };

        mBuilder.cores.push_back(core);
    }

    void addPackageInfo(const PROCESSOR_RELATIONSHIP &info) noexcept {
        PartialPackageInfo package = {
            .masks = sm::VectorBase(info.GroupMask, info.GroupMask + info.GroupCount)
        };

        mBuilder.pendingPackages.emplace_back(package);
    }

    void addCacheInfo(const CACHE_RELATIONSHIP &info) noexcept {
        uint id = mCacheId++;

        for (WORD i = 0; i < info.GroupCount; i++) {
            Cache cache = {
                .id = id,
                .mask = { info.GroupMasks[i] },
                .level = info.Level,
                .associativity = info.Associativity,
                .lineSize = info.LineSize,
                .size = info.CacheSize,
                .type = getCacheType(info.Type)
            };

            mBuilder.caches.push_back(cache);
        }
    }
};

struct CpuSetLayout {
    CpuSetLayout(GeometryBuilder& build) noexcept
        : builder(build)
    { }

    GeometryBuilder& builder;

    void addCpuSetInfo(const SYSTEM_CPU_SET_INFORMATION &info) noexcept {
        auto cpuset = info.CpuSet;
        GROUP_AFFINITY affinity = {
            .Mask = 1ull << KAFFINITY(cpuset.LogicalProcessorIndex),
            .Group = cpuset.Group,
        };

        // update schedules for all cores that are in this cpuset
        for (Core& core : builder.cores) {
            if (core.isContainedIn(affinity)) {
                core.schedule = cpuset.SchedulingClass;
            }
        }
    }
};
}

const CpuGeometry& threads::getCpuGeometry() noexcept {
    return gCpuGeometry;
}

void threads::init() noexcept {
    GeometryBuilder builder;
    ProcessorInfoLayout processorInfoLayout{builder};
    CpuSetLayout cpusetInfoLayout{builder};

    ProcessorInfo processorInfo{RelationAll, builder};
    CpuSetInfo cpusetInfo{builder};

    for (auto relation : processorInfo) {
        if (relation->Relationship == RelationProcessorCore)
            processorInfoLayout.addCoreInfo(relation->Processor);
    }

    for (auto relation : processorInfo) {
        if (relation->Relationship == RelationCache)
            processorInfoLayout.addCacheInfo(relation->Cache);
    }

    for (auto relation : processorInfo) {
        if (relation->Relationship == RelationProcessorPackage)
            processorInfoLayout.addPackageInfo(relation->Processor);
    }

    for (auto cpuset : cpusetInfo) {
        if (cpuset->Type == CpuSetInformation)
            cpusetInfoLayout.addCpuSetInfo(*cpuset);
    }

    // print cpu geometry

    gCpuGeometry = builder.build();
}
