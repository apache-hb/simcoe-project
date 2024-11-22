#include "threads/threads.hpp"

#include "backends/common.hpp"
#include <unordered_map>

#include "db/connection.hpp"

#include <immintrin.h>

#include "topology.dao.hpp"

using namespace sm::threads;

static std::vector<Cache> getCachesOfLevel(const std::vector<Cache>& caches, uint32_t level) {
    std::vector<Cache> result;
    for (const auto& cache : caches) {
        if (cache.level != level)
            continue;

        result.push_back(cache);
    }

    return result;
}

std::vector<Cache> ICpuGeometry::l3Caches() const {
    return getCachesOfLevel(mCaches, 3);
}

std::vector<Cache> ICpuGeometry::l2Caches() const {
    return getCachesOfLevel(mCaches, 2);
}

std::vector<Cache> ICpuGeometry::l1Caches() const {
    return getCachesOfLevel(mCaches, 1);
}

void ICpuGeometry::createCoreGroups() {
    std::unordered_map<WORD, Group> groups;

    for (const auto& core : mLogicalCores) {
        Group& group = groups[core.group];
        group.logicalCores.push_back(core);
    }

    for (auto& [id, group] : groups) {
        mCoreGroups.push_back(group);
    }
}

namespace topology = sm::dao::topology;

static std::string getUniqueComputerId() {
    const char *key = "SOFTWARE\\Microsoft\\Cryptography";
    const char *value = "MachineGuid";

    char buffer[256];
    DWORD size = sizeof(buffer);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, key, value, RRF_RT_REG_SZ, nullptr, buffer, &size) != ERROR_SUCCESS) {
        return "";
    }

    return buffer;
}

static std::string getProcessorString() {
    union {
        int data[12];
        uint32_t regs[12];
    };

    __cpuid(data, 0x80000000);

    if (regs[0] < 0x80000004)
        return "";

    __cpuid(data + 0, 0x80000002);
    __cpuid(data + 4, 0x80000003);
    __cpuid(data + 8, 0x80000004);

    char buffer[sizeof(regs) + 1];
    memcpy(buffer, regs, sizeof(regs));
    buffer[sizeof(buffer) - 1] = '\0';

    return buffer;
}

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

    auto computerId = getUniqueComputerId();
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
