#pragma once

#include "core/macros.hpp"
#include "threads.reflect.h"

#include "logs/logs.hpp"
#include <vector>

namespace sm::threads {
    enum struct SubcoreIndex : uint16_t { eInvalid = UINT16_MAX };
    enum struct CoreIndex : uint16_t { eInvalid = UINT16_MAX };
    enum struct ChipletIndex : uint16_t { eInvalid = UINT16_MAX };
    enum struct PackageIndex : uint16_t { eInvalid = UINT16_MAX };

    using SubcoreIndices = std::vector<SubcoreIndex>;
    using CoreIndices = std::vector<CoreIndex>;
    using ChipletIndices = std::vector<ChipletIndex>;
    using PackageIndices = std::vector<PackageIndex>;

    struct Subcore {
        GROUP_AFFINITY mask;
    };

    struct Core {
        GROUP_AFFINITY mask;
        SubcoreIndices subcores;

        uint16_t schedule; // schedule speed (lower is faster)
        uint8_t efficiency; // efficiency (higher is more efficient)
    };

    struct Chiplet {
        GROUP_AFFINITY mask;
        CoreIndices cores;
    };

    struct Package {
        GROUP_AFFINITY mask;

        CoreIndices cores;
        SubcoreIndices subcores;
        ChipletIndices chiplets;
    };

    struct CpuGeometry {
        // we call hardware threads "subcores" to avoid confusion with OS threads
        std::vector<Subcore> subcores;
        std::vector<Core> cores;
        std::vector<Chiplet> chiplets;
        std::vector<Package> packages;

        const Subcore& get_subcore(SubcoreIndex idx) const { return subcores[size_t(idx)]; }
        const Core& get_core(CoreIndex idx) const { return cores[size_t(idx)]; }
        const Chiplet& get_chiplet(ChipletIndex idx) const { return chiplets[size_t(idx)]; }
        const Package& get_package(PackageIndex idx) const { return packages[size_t(idx)]; }
    };

    CpuGeometry global_cpu_geometry(logs::ILogger& logger);

    class ThreadHandle {
        HANDLE m_handle = nullptr;

    public:
        ThreadHandle(HANDLE handle)
            : m_handle(handle)
        { }

        HANDLE get_handle() const { return m_handle; }
    };

    class Scheduler {
        CpuGeometry m_cpu;
        logs::Sink<logs::Category::eSchedule> m_log;

        static DWORD WINAPI thread_thunk(LPVOID param);

        ThreadHandle launch_thread(void* param);

    public:
        Scheduler(const SchedulerConfig&, CpuGeometry geometry, logs::ILogger& logger)
            : m_cpu(std::move(geometry))
            , m_log(logger)
        { }

        template<typename T>
        ThreadHandle launch(SM_UNUSED T&& task) {
            return ThreadHandle { nullptr };
        }
    };
}
