#include "stdafx.hpp"

#include "threads/topology.hpp"

#include "config/config.hpp"

#include "system/system.hpp"

#include "topology.dao.hpp"

using namespace sm;
using namespace sm::threads;

LOG_MESSAGE_CATEGORY(TopologyLog, "HWLOC");

static sm::opt<bool> gUsePinning {
    name = "pinning",
    desc = "Use core pinning",
    init = true
};

void HwlocTopology::destroyTopology(hwloc_topology_t topology) {
    hwloc_topology_destroy(topology);
}

HwlocTopology::HwlocTopology(hwloc_topology_t topology) noexcept
    : mTopology(HwlocTopology::Handle(topology, &destroyTopology))
{ }

HwlocTopology *HwlocTopology::fromSystem() {
    hwloc_topology_t topology;
    if (int err = hwloc_topology_init(&topology)) {
        LOG_ERROR(TopologyLog, "hwloc_topology_init failed with error {}", err);
        return nullptr;
    }

    if (int err = hwloc_topology_load(topology)) {
        defer { hwloc_topology_destroy(topology); };

        LOG_ERROR(TopologyLog, "hwloc_topology_load failed with error {} ({})", err, OsError(errno));
        return nullptr;
    }

    return new HwlocTopology(topology);
}


std::string HwlocTopology::exportToXml() const {
    char *xml = nullptr;
    int size = -1;
    if (int err = hwloc_topology_export_xmlbuffer(mTopology.get(), &xml, &size, 0); err) {
        LOG_ERROR(TopologyLog, "hwloc_topology_export_xmlbuffer failed with error {} ({})", err, OsError(errno));
        return "";
    }

    defer { hwloc_free_xmlbuffer(mTopology.get(), xml); };

    return std::string(xml, size - 1); // size includes the null terminator
}

HwlocTopology *HwlocTopology::fromXml(std::string_view xml) {
    hwloc_topology_t topology;
    if (int err = hwloc_topology_init(&topology)) {
        LOG_ERROR(TopologyLog, "hwloc_topology_init failed with error {}", err);
        return nullptr;
    }

    if (int err = hwloc_topology_set_xmlbuffer(topology, xml.data(), xml.size())) {
        defer { hwloc_topology_destroy(topology); };

        LOG_ERROR(TopologyLog, "hwloc_topology_set_xmlbuffer failed with error {} ({})", err, OsError(errno));
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
        if (os_error_t err = system::os::createThread(&handle, &threadId, start, param)) {
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

threads::IScheduler *HwlocTopology::newScheduler() {
    return new HwlocScheduler(mTopology.get());
}

void HwlocTopology::save(db::Connection& db) {
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

// /SYS/PM1/CMP0/DVRM_M0/V_+1V5 failed to read good POK state

// 0:0:0>Dimms Present:
// 0:0:0>NODE 0:
// 0:0:0>/SYS/PM0/CMP0/BOB0/CH0/D0 (J1701)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB1/CH0/D0 (J2501)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB0/CH1/D0 (J1901)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB1/CH1/D0 (J2701)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB2/CH0/D0 (J3301)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB3/CH0/D0 (J4101)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB2/CH1/D0 (J3501)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP0/BOB3/CH1/D0 (J4301)  Size = 00000004.00000000
// 0:0:0>NODE 1:
// 0:0:0>/SYS/PM0/CMP1/BOB2/CH1/D0 (J7801)  Size = 00000004.00000000
// 0:0:0>/SYS/PM0/CMP1/BOB3/CH1/D0 (J8601)  Size = 00000004.00000000
// 0:0:0>CMP 2998 MHz SMI 6.4 Gb/s DRAM 533MHz
// L3 Enabled
// ERROR:   POST Timed out.Not all system components tested.

// seems BOB0 and BOB1 on CMP1 arent responding to POST
