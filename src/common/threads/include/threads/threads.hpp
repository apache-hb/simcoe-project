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

    // each subcore belongs to only a single core.
    enum struct ThreadIndex : uint16 {
        eInvalid = UINT16_MAX
    };

    // a core will have 1 or more threads, and belongs to a single chiplet.
    enum struct CoreIndex : uint16 {
        eInvalid = UINT16_MAX
    };

    // a group will have 1 or more cores, and belongs to a single package.
    enum struct GroupIndex : uint16 {
        eInvalid = UINT16_MAX
    };

    // a package will have 1 or more chiplets
    enum struct PackageIndex : uint16 {
        eInvalid = UINT16_MAX
    };

    // a cache will have 1 or more segments, and belongs to a single package.
    // it may span multiple groups, and will be identified by the same id.
    enum struct CacheIndex : uint16 {
        eInvalid = UINT16_MAX
    };

    template<typename T> requires __is_enum(T)
    struct IndexRange {
        T front;
        T back;

        constexpr size_t size() const noexcept {
            return static_cast<size_t>(back) - static_cast<size_t>(front) + 1;
        }
    };

    // scheduling mask for a single group
    struct ScheduleMask {
        GROUP_AFFINITY mask;

        constexpr bool isContainedIn(GROUP_AFFINITY other) const noexcept {
            // check if the group is the same and that our mask is a subset of the other mask
            return (mask.Group == other.Group) && ((mask.Mask & other.Mask) == mask.Mask);
        }
    };

    // scheduling mask that can span multiple groups
    struct GroupScheduleMask {
        std::bitset<kMaxThreads> mask;
    };

    struct LogicalCore : ScheduleMask { };

    struct Cache {
        uint id; // multiple caches may share the same id if they span multiple groups
        ScheduleMask mask; // the mask for this segment of the cache

        uint8 level;
        uint8 associativity;
        uint16 lineSize;
        uint32 size;
        CacheType type;
    };

    struct Core : ScheduleMask {
        IndexRange<ThreadIndex> threads;

        uint16 schedule;  // schedule speed (lower is faster)
        uint8 efficiency; // efficiency (higher is more efficient)
    };

    struct Group : ScheduleMask {
        IndexRange<CoreIndex> cores;
    };

    struct Package {
        IndexRange<ThreadIndex> threads;
        IndexRange<CoreIndex> cores;
        IndexRange<GroupIndex> groups;
        IndexRange<CacheIndex> caches;
    };

    struct CpuGeometry {
        VectorBase<LogicalCore> threads;
        VectorBase<Core> cores;
        VectorBase<Cache> caches;
        VectorBase<Group> groups;
        VectorBase<Package> packages;
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
