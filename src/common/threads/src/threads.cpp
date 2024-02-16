#include "threads/threads.hpp"

#include "core/unique.hpp"
#include "core/span.hpp"

#include "system/system.hpp"

#include "logs/sink.inl" // IWYU pragma: keep

#include <errhandlingapi.h>
#include <processthreadsapi.h>
#include <sysinfoapi.h>
#include <unordered_set>
#include <winerror.h>

using namespace sm;
using namespace sm::threads;

template <typename T>
T *advance(T *ptr, size_t bytes) {
    return reinterpret_cast<T *>(reinterpret_cast<std::byte *>(ptr) + bytes);
}

struct ProcessorInfoIterator {
    ProcessorInfoIterator(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buffer, DWORD remaining)
        : m_buffer(buffer), m_remaining(remaining) {
    }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator++() {
        CTASSERT(m_remaining > 0);

        m_remaining -= m_buffer->Size;
        if (m_remaining > 0) {
            m_buffer = advance(m_buffer, m_buffer->Size);
        }

        return m_buffer;
    }

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *operator*() const {
        return m_buffer;
    }

    bool operator==(const ProcessorInfoIterator &other) const {
        return m_remaining == other.m_remaining;
    }

  private:
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *m_buffer;
    DWORD m_remaining;
};

// GetLogicalProcessorInformationEx provides information about inter-core communication
struct ProcessorInfo {
    ProcessorInfo(LOGICAL_PROCESSOR_RELATIONSHIP relation) {
        SM_ASSERT_WIN32(!GetLogicalProcessorInformationEx(relation, nullptr, &m_size));
        SM_ASSERT_WIN32(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        m_memory = sm::UniquePtr<std::byte[]>(m_size);
        m_buffer = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>(m_memory.get());
        SM_ASSERT_WIN32(GetLogicalProcessorInformationEx(relation, m_buffer, &m_size));

        m_remaining = m_size;
    }

    ProcessorInfoIterator begin() const {
        return ProcessorInfoIterator(m_buffer, m_remaining);
    }

    ProcessorInfoIterator end() const {
        return ProcessorInfoIterator(m_buffer, 0);
    }

  private:
    sm::UniquePtr<std::byte[]> m_memory;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *m_buffer;
    DWORD m_size = 0;

    DWORD m_remaining = 0;
};

// GetSystemCpuSetInformation provides information about cpu speeds
struct CpuSetIterator {
    CpuSetIterator(SYSTEM_CPU_SET_INFORMATION *pBuffer, ULONG remaining)
        : m_buffer(pBuffer), m_remaining(remaining) {
    }

    SYSTEM_CPU_SET_INFORMATION *operator++() {
        CTASSERT(m_remaining > 0);

        m_remaining -= m_buffer->Size;
        if (m_remaining > 0) {
            m_buffer = advance(m_buffer, m_buffer->Size);
        }

        return m_buffer;
    }

    SYSTEM_CPU_SET_INFORMATION *operator*() const {
        return m_buffer;
    }

    bool operator==(const CpuSetIterator &other) const {
        return m_remaining == other.m_remaining;
    }

  private:
    SYSTEM_CPU_SET_INFORMATION *m_buffer;
    ULONG m_remaining;
};

struct CpuSetInfo {
    CpuSetInfo() {
        HANDLE process = GetCurrentProcess();
        SM_ASSERT_WIN32(!GetSystemCpuSetInformation(nullptr, 0, &m_size, process, 0));

        SM_ASSERT_WIN32(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        m_memory = sm::UniquePtr<std::byte[]>(m_size);
        m_buffer = reinterpret_cast<SYSTEM_CPU_SET_INFORMATION *>(m_memory.get());
        SM_ASSERT_WIN32(GetSystemCpuSetInformation(m_buffer, m_size, &m_size, process, 0));

        m_remaining = m_size;
    }

    CpuSetIterator begin() const {
        return CpuSetIterator(m_buffer, m_remaining);
    }

    CpuSetIterator end() const {
        return CpuSetIterator(m_buffer, 0);
    }

  private:
    sm::UniquePtr<std::byte[]> m_memory;
    SYSTEM_CPU_SET_INFORMATION *m_buffer;
    ULONG m_size = 0;

    ULONG m_remaining = 0;
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
                ids.push_back(sm::enum_cast<Index>(i));

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
    ProcessorInfoLayout(GeometryBuilder *bundler) : m_builder(bundler) {
    }

    GeometryBuilder *m_builder;

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

                m_builder->subcores.push_back({ .mask = groupAffinity });

                subcore_ids.push_back(sm::enum_cast<SubcoreIndex>(m_builder->subcores.size() - 1));
            }
        }

        m_builder->cores.push_back({
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
            m_builder->get_masked_cores(core_ids, group);
            m_builder->get_masked_subcores(subcore_ids, group);
            m_builder->get_masked_chiplets(chiplet_ids, group);
        }

        m_builder->packages.push_back({
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
            m_builder->get_masked_cores(core_ids, group);
        }

        m_builder->chiplets.push_back({
            .mask = info.GroupMasks[0],
            .cores = core_ids,
        });
    }
};

struct CpuSetLayout {
    CpuSetLayout(GeometryBuilder *bundler) : m_builder(bundler) {
    }

    GeometryBuilder *m_builder;

    void add_cpuset(const SYSTEM_CPU_SET_INFORMATION &info) {
        auto cpuset = info.CpuSet;
        GROUP_AFFINITY affinity = {
            .Mask = 1ull << KAFFINITY(cpuset.LogicalProcessorIndex),
            .Group = cpuset.Group,
        };

        CoreIndices cores;
        m_builder->get_masked_cores(cores, affinity);
        for (CoreIndex coreId : cores) {
            m_builder->cores[size_t(coreId)].schedule = cpuset.SchedulingClass;
        }
    }
};

CpuGeometry threads::global_cpu_geometry(logs::ILogger &logger) {
    logs::Sink<logs::Category::eSchedule> log{logger};
    GeometryBuilder builder;
    ProcessorInfoLayout processor_layout{&builder};
    CpuSetLayout cpuset_layout{&builder};

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

    log.info("cpu geometry: {} package(s), {} chiplet(s), {} cores, {} subcores",
             builder.packages.size(), builder.chiplets.size(), builder.cores.size(),
             builder.subcores.size());

    return builder;
}
