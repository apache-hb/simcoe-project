#include "stdafx.hpp"

#include "common.hpp"

#include "core/defer.hpp"

#include "db/connection.hpp"

#include "system/system.hpp"

#include "topology.dao.hpp"

using namespace sm;

namespace detail = sm::threads::detail;

LOG_MESSAGE_CATEGORY(TopologyLog, "HWLOC");

detail::HwlocTopology *detail::hwlocInit() {
    hwloc_topology_t topology;
    if (int err = hwloc_topology_init(&topology); err) {
        LOG_ERROR(TopologyLog, "hwloc_topology_init failed with error {}", err);
        return nullptr;
    }

    if (int err = hwloc_topology_load(topology); err) {
        defer { hwloc_topology_destroy(topology); };

        LOG_ERROR(TopologyLog, "hwloc_topology_load failed with error {} ({})", err, OsError(errno));
        return nullptr;
    }

    return new HwlocTopology(topology);
}

class HwlocScheduler final : public threads::IScheduler {
    [[maybe_unused]]
    hwloc_topology_t mTopology;

    threads::ThreadHandle launchThread(void *param, system::os::StartRoutine start) override {
        system::os::Thread handle = system::os::kInvalidThread;
        system::os::ThreadId threadId = 0;
        if (os_error_t err = system::os::createThread(&handle, &threadId, start, param); err) {
            LOG_ERROR(TopologyLog, "createThread failed with error {}", err);
            throw OsException(err, SM_OS_CREATE_THREAD);
        }

        return threads::ThreadHandle(handle, threadId);
    }

public:
    HwlocScheduler(hwloc_topology_t topology) noexcept
        : mTopology(topology)
    { }
};

threads::IScheduler *detail::HwlocTopology::newScheduler() {
    return new HwlocScheduler(mTopology.get());
}

void detail::HwlocTopology::save(db::Connection& db) {
    char *xml = nullptr;
    int size = -1;
    if (int err = hwloc_topology_export_xmlbuffer(mTopology.get(), &xml, &size, 0); err) {
        LOG_ERROR(TopologyLog, "hwloc_topology_export_xmlbuffer failed with error {} ({})", err, OsError(errno));
        return;
    }

    defer { hwloc_free_xmlbuffer(mTopology.get(), xml); };

    db.createTable(dao::topology::HwlocTopology::table());
    db.insertOrUpdate(dao::topology::HwlocTopology {
        .os = system::getMachineId(),
        .hash = std::hash<std::string_view>{}(std::string_view(xml, size - 1)), // size includes the null terminator
        .data = db::Blob((uint8_t*)xml, (uint8_t*)xml + size - 1)
    });
}
