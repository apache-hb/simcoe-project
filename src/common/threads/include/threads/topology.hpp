#pragma once

#include "threads/scheduler.hpp"

typedef struct hwloc_topology *hwloc_topology_t;

namespace sm::threads {
    class ITopology {
    public:
        virtual ~ITopology() = default;

        virtual IScheduler *newScheduler() = 0;
        virtual void save(db::Connection& connection) = 0;
    };

    class HwlocTopology final : public ITopology {
        static void destroyTopology(hwloc_topology_t topology);
        using Handle = std::unique_ptr<hwloc_topology, decltype(&destroyTopology)>;

        Handle mTopology;

    public:
        HwlocTopology(hwloc_topology_t topology) noexcept;

        IScheduler *newScheduler() override;
        void save(db::Connection& db) override;

        std::string exportToXml() const;
        static HwlocTopology *fromXml(std::string_view xml);
        static HwlocTopology *fromSystem();
    };

    void saveThreadInfo(db::Connection& connection);
}
