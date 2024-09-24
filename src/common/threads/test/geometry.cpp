#include "test/common.hpp"

#include "threads/threads.hpp"
#include "common.hpp"

using namespace sm;

using CpuInfoLibrary = threads::detail::CpuInfoLibrary;

TEST_CASE("Processor Geometry") {
    threads::init();

    const auto& geometry = threads::getCpuGeometry();

    CHECK(geometry.packages.size() > 0);
    // CHECK(geometry.cores.size() > 0);
    // CHECK(geometry.threads.size() > 0);
    CHECK(geometry.caches.size() > 0);
}

TEST_CASE("New APIs missing") {
    CpuInfoLibrary lib = CpuInfoLibrary::load();

    CpuInfoLibrary temp = {
        .pfnGetSystemCpuSetInformation = lib.pfnGetSystemCpuSetInformation,
        .pfnGetLogicalProcessorInformationEx = lib.pfnGetLogicalProcessorInformationEx
    };

    THEN("CPU Geometry is built correctly with all apis present") {
        auto result = threads::detail::buildCpuGeometry(temp);
        CHECK(result.processorCores.size() > 0);
        CHECK(result.logicalCores.size() > 0);
        CHECK(result.packages.size() > 0);
    }

    THEN("CPU Geometry is built correctly without GetSystemCpuSetInformation") {
        temp.pfnGetSystemCpuSetInformation = nullptr;
        auto result = threads::detail::buildCpuGeometry(temp);
        CHECK(result.processorCores.size() > 0);
        CHECK(result.logicalCores.size() > 0);
        CHECK(result.packages.size() > 0);
    }

    THEN("CPU Geometry is built correctly without GetLogicalProcessorInformationEx") {
        temp.pfnGetSystemCpuSetInformation = nullptr;
        temp.pfnGetLogicalProcessorInformationEx = nullptr;
        auto result = threads::detail::buildCpuGeometry(temp);
        CHECK(result.processorCores.size() > 0);
        CHECK(result.logicalCores.size() > 0);
        CHECK(result.packages.size() > 0);
    }
}