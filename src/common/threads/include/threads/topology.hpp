#pragma once

#include "threads/scheduler.hpp"

namespace sm::threads {
        // scheduling mask for a single group
    struct ScheduleMask {
        GROUP_AFFINITY mask;

        constexpr bool isContainedIn(GROUP_AFFINITY other) const noexcept {
            // check if the group is the same and that our mask is a subset of the other mask
            return (mask.Group == other.Group) && ((mask.Mask & other.Mask) == mask.Mask);
        }
    };

    struct LogicalCore {
        DWORD id;
        WORD group;
        WORD coreIndex;
        WORD threadIndex;
    };

    struct ProcessorCore {
        uint16 schedule;  // schedule speed (lower is faster)
        uint8 efficiency; // efficiency (higher is more efficient)
    };

    struct Cache {
        uint8 level;
        uint8 associativity;
        uint16 lineSize;
        uint32 size;
        CacheType type;
    };

    struct CoreChiplet {

    };

    struct ProcessorPackage {
        std::vector<ScheduleMask> groups;
    };

    struct Group {
        Cache l3Cache;
        std::vector<Cache> l2Caches;
        std::vector<Cache> l1Caches;
        std::vector<LogicalCore> logicalCores;
        std::vector<ProcessorCore> cores;
    };

    struct NumaNode {

    };

    struct ProcessorModule {

    };

    class ICpuGeometry {

    protected:
        std::vector<LogicalCore> mLogicalCores;
        std::vector<ProcessorCore> mProcessorCores;
        std::vector<Cache> mCaches;

        // amd ccd groups
        std::vector<Group> mCoreGroups;

        void createCoreGroups();

        ICpuGeometry() = default;

    public:
        virtual ~ICpuGeometry() = default;

        virtual IScheduler *newScheduler(std::vector<LogicalCore> threads) = 0;

        std::span<const LogicalCore> logicalCores() const noexcept { return mLogicalCores; }
        std::span<const ProcessorCore> processorCores() const noexcept { return mProcessorCores; }
        std::span<const Cache> caches() const noexcept { return mCaches; }

        std::vector<Cache> l3Caches() const;
        std::vector<Cache> l2Caches() const;
        std::vector<Cache> l1Caches() const;
    };

    ICpuGeometry *getCpuGeometry() noexcept;

    void saveThreadInfo(db::Connection& connection);
}
