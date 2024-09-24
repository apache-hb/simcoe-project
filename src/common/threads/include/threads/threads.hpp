#pragma once

#include "core/win32.hpp" // IWYU pragma: keep

#include "core/macros.hpp"
#include "core/adt/vector.hpp"
#include "core/core.hpp"

#include <bitset>

#include "threads.meta.hpp"

namespace sm::threads {
    static constexpr size_t kMaxThreads = 512;

    REFLECT_ENUM(ThreadClass)
    enum class ThreadClass : uint8 {
        eRealtime,
        eNormal,
        eWorker,
        eIdle
    };

    REFLECT_ENUM(PriorityClass)
    enum class PriorityClass : int16 {
        eHigh = HIGH_PRIORITY_CLASS,
        eNormal = NORMAL_PRIORITY_CLASS,
        eIdle = THREAD_PRIORITY_IDLE,
    };

    REFLECT_ENUM(CacheType)
    enum class CacheType : uint8 {
        eUnified,
        eInstruction,
        eData,
        eTrace,
        eUnknown
    };

    // scheduling mask for a single group
    struct ScheduleMask {
        GROUP_AFFINITY mask;

        constexpr bool isContainedIn(GROUP_AFFINITY other) const noexcept {
            // check if the group is the same and that our mask is a subset of the other mask
            return (mask.Group == other.Group) && ((mask.Mask & other.Mask) == mask.Mask);
        }
    };

    struct LogicalCore : ScheduleMask {

    };

    struct ProcessorCore : ScheduleMask {
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

    struct ProcessorPackage {

    };

    struct Group {

    };

    struct ProcessorDie {

    };

    struct NumaNode {

    };

    struct ProcessorModule {

    };

    struct CpuGeometry {
        std::vector<LogicalCore> logicalCores;
        std::vector<ProcessorCore> processorCores;
        std::vector<Cache> caches;
        std::vector<ProcessorPackage> packages;
        std::vector<Group> groups;
        std::vector<ProcessorDie> dies;
        std::vector<NumaNode> numaNodes;
        std::vector<ProcessorModule> modules;

        // if we can't detect the cpu geometry, just let the OS
        // handle scheduling for us.
        bool disableScheduler() const noexcept {
            return logicalCores.size() == 0
                || processorCores.size() == 0;
        }
    };

    void init() noexcept;
    const CpuGeometry& getCpuGeometry() noexcept;

    class ThreadHandle {
        HANDLE mHandle = nullptr;

    public:
        ThreadHandle(HANDLE handle)
            : mHandle(handle)
        { }

        HANDLE getHandle() const {
            return mHandle;
        }
    };

    struct SchedulerConfig {
        uint workers;
        PriorityClass priority;
    };

    class Scheduler {
        SchedulerConfig mConfig;
        CpuGeometry mCpuGeometry;

        static DWORD WINAPI threadThunk(LPVOID param);

        ThreadHandle launchThread(void *param);

    public:
        Scheduler(SchedulerConfig config, CpuGeometry geometry)
            : mConfig(config)
            , mCpuGeometry(std::move(geometry))
        { }

        template <typename T>
        ThreadHandle launch(SM_UNUSED T &&task) {
            return ThreadHandle{nullptr};
        }
    };
}
