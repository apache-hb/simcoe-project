#pragma once

#include "core/macros.hpp"
#include "core/vector.hpp"

#include "threads.reflect.h"

namespace sm::threads {
enum struct SubcoreIndex : uint16_t {
    eInvalid = UINT16_MAX
};
enum struct CoreIndex : uint16_t {
    eInvalid = UINT16_MAX
};
enum struct ChipletIndex : uint16_t {
    eInvalid = UINT16_MAX
};
enum struct PackageIndex : uint16_t {
    eInvalid = UINT16_MAX
};

using SubcoreIndices = sm::Vector<SubcoreIndex>;
using CoreIndices = sm::Vector<CoreIndex>;
using ChipletIndices = sm::Vector<ChipletIndex>;
using PackageIndices = sm::Vector<PackageIndex>;

struct Subcore {
    GROUP_AFFINITY mask;
};

struct Core {
    GROUP_AFFINITY mask;
    SubcoreIndices subcores;

    uint16_t schedule;  // schedule speed (lower is faster)
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
    sm::Vector<Subcore> subcores;
    sm::Vector<Core> cores;
    sm::Vector<Chiplet> chiplets;
    sm::Vector<Package> packages;

    const Subcore &get_subcore(SubcoreIndex idx) const {
        return subcores[size_t(idx)];
    }
    const Core &get_core(CoreIndex idx) const {
        return cores[size_t(idx)];
    }
    const Chiplet &get_chiplet(ChipletIndex idx) const {
        return chiplets[size_t(idx)];
    }
    const Package &get_package(PackageIndex idx) const {
        return packages[size_t(idx)];
    }
};

CpuGeometry global_cpu_geometry();

class ThreadHandle {
    HANDLE m_handle = nullptr;

public:
    ThreadHandle(HANDLE handle)
        : m_handle(handle) {}

    HANDLE get_handle() const {
        return m_handle;
    }
};

class Scheduler {
    CpuGeometry m_cpu;

    static DWORD WINAPI thread_thunk(LPVOID param);

    ThreadHandle launch_thread(void *param);

public:
    Scheduler(const SchedulerConfig &, CpuGeometry geometry)
        : m_cpu(std::move(geometry))
    { }

    template <typename T>
    ThreadHandle launch(SM_UNUSED T &&task) {
        return ThreadHandle{nullptr};
    }
};
} // namespace sm::threads
