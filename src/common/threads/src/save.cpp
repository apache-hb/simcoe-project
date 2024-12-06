#include "threads/threads.hpp"

#include "cpuinfo.hpp"

#include "system/system.hpp"

#include "db/connection.hpp"

#include "core/cpuid.hpp"

#include "topology.dao.hpp"

using namespace sm::threads;

namespace topology = sm::dao::topology;

static std::string getProcessorString() {
    char brand[sm::kBrandStringSize];
    if (!sm::CpuId::getBrandString(brand))
        return "";

    return std::string(brand, sizeof(brand));
}

// TODO: should use a proper hash library rather than this
static uint64_t hashBlob(std::span<const uint8_t> data) {
    uint64_t hash = 0x84222325;
    for (uint8_t byte : data) {
        hash ^= byte;
        hash *= 0x100000001b3;
    }

    return hash;
}

void sm::threads::saveThreadInfo(db::Connection& connection) {
    connection.createTable(topology::CpuSetInfo::table());
    connection.createTable(topology::LogicalProcessorInfo::table());

    detail::CpuInfoLibrary library = detail::CpuInfoLibrary::load();
    auto [cpuSetData, cpuSetSize] = detail::readSystemCpuSetInformation(library.pfnGetSystemCpuSetInformation);
    auto [layoutData, layoutSize] = detail::readLogicalProcessorInformationEx(library.pfnGetLogicalProcessorInformationEx, RelationAll);

    auto computerId = system::getMachineId();
    auto processor = getProcessorString();

    if (cpuSetData != nullptr) {
        std::vector<uint8_t> data { cpuSetData.get(), cpuSetData.get() + cpuSetSize };
        topology::CpuSetInfo dao {
            .cpu = processor,
            .os = computerId,
            .hash = hashBlob(data),
            .data = data,
        };

        connection.insertReturningPrimaryKey(dao);
    }

    if (layoutData != nullptr) {
        std::vector<uint8_t> data { layoutData.get(), layoutData.get() + layoutSize };
        topology::LogicalProcessorInfo dao {
            .cpu = processor,
            .os = computerId,
            .hash = hashBlob(data),
            .data = data,
        };

        connection.insertReturningPrimaryKey(dao);
    }
}
