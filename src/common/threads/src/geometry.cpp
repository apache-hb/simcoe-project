#include "threads/threads.hpp"

#include "core/unique.hpp"
#include "core/span.hpp"

#include "system/system.hpp"

#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <unordered_set>
#include <winerror.h>

using namespace sm;
using namespace sm::threads;

static auto gSink = logs::get_sink(logs::Category::eSchedule);

template <typename T>
static T *advance(T *ptr, size_t bytes) {
    return reinterpret_cast<T *>(reinterpret_cast<std::byte *>(ptr) + bytes);
}

struct ProcessorInfoIterator {
    ProcessorInfoIterator(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD remaining)
        : mBuffer(buffer)
        , mRemaining(remaining)
    { }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator++() {
        CTASSERT(mRemaining > 0);

        mRemaining -= mBuffer->Size;
        if (mRemaining > 0) {
            mBuffer = advance(mBuffer, mBuffer->Size);
        }

        return mBuffer;
    }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator*() const {
        return mBuffer;
    }

    bool operator==(const ProcessorInfoIterator &other) const {
        return mRemaining == other.mRemaining;
    }

  private:
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *mBuffer;
    DWORD mRemaining;
};

// GetLogicalProcessorInformationEx provides information about inter-core communication
struct ProcessorInfo {
    ProcessorInfo(LOGICAL_PROCESSOR_RELATIONSHIP relation) {
        SM_ASSERT_WIN32(!GetLogicalProcessorInformationEx(relation, nullptr, &mSize));
        SM_ASSERT_WIN32(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        mMemory = sm::UniquePtr<std::byte[]>(mSize);
        mBuffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(mMemory.get());
        SM_ASSERT_WIN32(GetLogicalProcessorInformationEx(relation, mBuffer, &mSize));

        mRemaining = mSize;
    }

    ProcessorInfoIterator begin() const {
        return ProcessorInfoIterator(mBuffer, mRemaining);
    }

    ProcessorInfoIterator end() const {
        return ProcessorInfoIterator(mBuffer, 0);
    }

  private:
    sm::UniquePtr<std::byte[]> mMemory;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *mBuffer;
    DWORD mSize = 0;

    DWORD mRemaining = 0;
};

// GetSystemCpuSetInformation provides information about cpu speeds
struct CpuSetIterator {
    CpuSetIterator(SYSTEM_CPU_SET_INFORMATION *pBuffer, ULONG remaining)
        : mBuffer(pBuffer)
        , mRemaining(remaining)
    { }

    SYSTEM_CPU_SET_INFORMATION *operator++() {
        CTASSERT(mRemaining > 0);

        mRemaining -= mBuffer->Size;
        if (mRemaining > 0) {
            mBuffer = advance(mBuffer, mBuffer->Size);
        }

        return mBuffer;
    }

    SYSTEM_CPU_SET_INFORMATION *operator*() const {
        return mBuffer;
    }

    bool operator==(const CpuSetIterator &other) const {
        return mRemaining == other.mRemaining;
    }

  private:
    SYSTEM_CPU_SET_INFORMATION *mBuffer;
    ULONG mRemaining;
};

struct CpuSetInfo {
    CpuSetInfo() {
        HANDLE process = GetCurrentProcess();
        SM_ASSERT_WIN32(!GetSystemCpuSetInformation(nullptr, 0, &mSize, process, 0));

        SM_ASSERT_WIN32(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        mMemory = sm::UniquePtr<std::byte[]>(mSize);
        mInfo = reinterpret_cast<SYSTEM_CPU_SET_INFORMATION *>(mMemory.get());
        SM_ASSERT_WIN32(GetSystemCpuSetInformation(mInfo, mSize, &mSize, process, 0));

        mRemaining = mSize;
    }

    CpuSetIterator begin() const {
        return CpuSetIterator(mInfo, mRemaining);
    }

    CpuSetIterator end() const {
        return CpuSetIterator(mInfo, 0);
    }

  private:
    sm::UniquePtr<std::byte[]> mMemory;
    SYSTEM_CPU_SET_INFORMATION *mInfo;
    ULONG mSize = 0;

    ULONG mRemaining = 0;
};

struct GeometryBuilder : CpuGeometry {
    std::unordered_set<uint16_t> unique;

    bool should_add_item(GROUP_AFFINITY item, GROUP_AFFINITY affinity) {
        if (item.Group != affinity.Group || !(item.Mask & affinity.Mask)) return false;

        auto [it, exists] = unique.emplace(item.Mask);
        return !exists;
    }

    // TODO: de-template this to save binary space
    template <typename Index, typename Item>
    void get_item_with_mask(sm::Vector<Index> &ids, Span<const Item> items,
                            GROUP_AFFINITY affinity) {
        for (size_t i = 0; i < items.size(); ++i)
            if (should_add_item(items[i].mask, affinity))
                ids.push_back(enum_cast<Index>(i));

        unique.clear();
    }

    // TODO: figure out the max number of subcores for the cpu and do this all with arrays

    void get_masked_subcores(SubcoreIndices &ids, GROUP_AFFINITY affinity) {
        get_item_with_mask<SubcoreIndex, Subcore>(ids, subcores, affinity);
    }

    void get_masked_cores(CoreIndices &ids, GROUP_AFFINITY affinity) {
        get_item_with_mask<CoreIndex, Core>(ids, cores, affinity);
    }

    void get_masked_chiplets(ChipletIndices &ids, GROUP_AFFINITY affinity) {
        get_item_with_mask<ChipletIndex, Chiplet>(ids, chiplets, affinity);
    }
};

struct ProcessorInfoLayout {
    ProcessorInfoLayout(GeometryBuilder& build)
        : builder(build)
    { }

    GeometryBuilder &builder;

    static constexpr KAFFINITY kAffinityBits = std::numeric_limits<KAFFINITY>::digits;

    void add_core(const PROCESSOR_RELATIONSHIP &info) {
        SubcoreIndices subcore_ids;

        for (DWORD i = 0; i < info.GroupCount; ++i) {
            GROUP_AFFINITY group = info.GroupMask[i];

            for (size_t bit = 0; bit < kAffinityBits; ++bit) {
                KAFFINITY mask = 1ull << bit;
                if (!(group.Mask & mask)) continue;

                GROUP_AFFINITY groupAffinity = {
                    .Mask = group.Mask & mask,
                    .Group = group.Group,
                };

                builder.subcores.push_back({ .mask = groupAffinity });

                subcore_ids.push_back(enum_cast<SubcoreIndex>(builder.subcores.size() - 1));
            }
        }

        builder.cores.push_back({
            .mask = info.GroupMask[0],
            .subcores = subcore_ids,
            .efficiency = info.EfficiencyClass,
        });
    }

    void add_package(const PROCESSOR_RELATIONSHIP &info) {
        SubcoreIndices subcore_ids;
        CoreIndices core_ids;
        ChipletIndices chiplet_ids;

        for (DWORD i = 0; i < info.GroupCount; ++i) {
            GROUP_AFFINITY group = info.GroupMask[i];
            builder.get_masked_cores(core_ids, group);
            builder.get_masked_subcores(subcore_ids, group);
            builder.get_masked_chiplets(chiplet_ids, group);
        }

        builder.packages.push_back({
            .mask = info.GroupMask[0],
            .cores = core_ids,
            .subcores = subcore_ids,
            .chiplets = chiplet_ids,
        });
    }

    void add_cache(const CACHE_RELATIONSHIP &info) {
        if (info.Level != 3) return;

        // assume everything that shares l3 cache is in the same cluster
        // TODO: on intel E-cores and P-cores share the same l3
        //       but we dont want to put them in the same cluster.
        //       for now that isnt a big problem though.
        CoreIndices core_ids;

        for (DWORD i = 0; i < info.GroupCount; ++i) {
            GROUP_AFFINITY group = info.GroupMasks[i];
            builder.get_masked_cores(core_ids, group);
        }

        builder.chiplets.push_back({
            .mask = info.GroupMasks[0],
            .cores = core_ids,
        });
    }
};

struct CpuSetLayout {
    CpuSetLayout(GeometryBuilder& build)
        : builder(build)
    { }

    GeometryBuilder& builder;

    void add_cpuset(const SYSTEM_CPU_SET_INFORMATION &info) {
        auto cpuset = info.CpuSet;
        GROUP_AFFINITY affinity = {
            .Mask = 1ull << KAFFINITY(cpuset.LogicalProcessorIndex),
            .Group = cpuset.Group,
        };

        CoreIndices cores;
        builder.get_masked_cores(cores, affinity);
        for (CoreIndex coreId : cores) {
            builder.cores[size_t(coreId)].schedule = cpuset.SchedulingClass;
        }
    }
};

CpuGeometry threads::global_cpu_geometry() {
    GeometryBuilder builder;
    ProcessorInfoLayout processor_layout{builder};
    CpuSetLayout cpuset_layout{builder};

    ProcessorInfo processor_info{RelationAll};
    CpuSetInfo cpuset_info{};

    for (const auto *relation : processor_info) {
        if (relation->Relationship == RelationProcessorCore)
            processor_layout.add_core(relation->Processor);
    }

    for (const auto *relation : processor_info) {
        if (relation->Relationship == RelationCache) processor_layout.add_cache(relation->Cache);
    }

    for (const auto *relation : processor_info) {
        if (relation->Relationship == RelationProcessorPackage)
            processor_layout.add_package(relation->Processor);
    }

    for (const auto *cpuset : cpuset_info) {
        if (cpuset->Type == CpuSetInformation) cpuset_layout.add_cpuset(*cpuset);
    }

    // print cpu geometry

    gSink.info("cpu geometry: {} package(s), {} chiplet(s), {} cores, {} subcores",
             builder.packages.size(), builder.chiplets.size(), builder.cores.size(),
             builder.subcores.size());

    return builder;
}
