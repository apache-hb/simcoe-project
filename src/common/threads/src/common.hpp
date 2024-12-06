#pragma once

#include "threads/topology.hpp"

#include <hwloc.h>

namespace sm::threads::detail {
    using HwlocTopologyHandle = std::unique_ptr<hwloc_topology, decltype(&hwloc_topology_destroy)>;

    class HwlocTopology final : public ITopology {
        HwlocTopologyHandle mTopology;

    public:
        HwlocTopology(hwloc_topology_t topology) noexcept
            : mTopology(HwlocTopologyHandle(topology, hwloc_topology_destroy))
        { }

        IScheduler *newScheduler() override;
        void save(db::Connection& db) override;
    };

    HwlocTopology *hwlocInit();
}
