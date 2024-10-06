#pragma once

#include "core/core.hpp"
#include "core/error.hpp"
#include "core/macros.hpp"
#include "core/memory.hpp"
#include "core/memory/unique.hpp"

#include "core/win32.hpp" // IWYU pragma: keep

#include "logs/structured/logging.hpp"

#include "threads.meta.hpp"

LOG_MESSAGE_CATEGORY(ThreadLog, "Threads");

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

    class ThreadHandle {
        HANDLE mHandle;
        DWORD mThreadId;

    public:
        ThreadHandle(HANDLE handle, DWORD threadId) noexcept;

        ~ThreadHandle() noexcept;

        OsError join() noexcept;

        HANDLE getHandle() const noexcept { return mHandle; }
        DWORD getThreadId() const noexcept { return mThreadId; }
    };

    class IScheduler {
        virtual ThreadHandle launchThread(void *param, LPTHREAD_START_ROUTINE start) = 0;

    public:
        virtual ~IScheduler() = default;

        template<typename F>
        ThreadHandle launch(F &&fn) {
            auto thunk = [](void *param) noexcept -> unsigned long {
                auto fn = sm::makeUnique<F>(reinterpret_cast<F*>(param));

                try {
                    (*fn)();
                    return 0;
                } catch (std::exception &err) {
                    LOG_ERROR(ThreadLog, "Thread exception: {}", err.what());
                } catch (...) {
                    LOG_ERROR(ThreadLog, "Unknown thread exception");
                }

                return -1;
            };

            F *param = new F(std::forward<F>(fn));
            return launchThread(param, thunk);
        }
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

    void create(void);
    void destroy(void) noexcept;
    ICpuGeometry *getCpuGeometry() noexcept;
}
