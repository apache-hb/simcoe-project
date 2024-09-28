#include "test/common.hpp"

#include "threads/threads.hpp"
#include "backends/common.hpp"

using namespace sm;

using CpuInfoLibrary = threads::detail::CpuInfoLibrary;

TEST_CASE("CpuSet Geometry") {
    CpuInfoLibrary lib = CpuInfoLibrary::load();
    if (lib.pfnGetLogicalProcessorInformationEx == nullptr)
        SKIP("GetLogicalProcessorInformationEx not available");

    if (lib.pfnGetSystemCpuSetInformation == nullptr)
        SKIP("GetSystemCpuSetInformation not available");

    auto result = threads::detail::newCpuSetGeometry(
        lib.pfnGetLogicalProcessorInformationEx,
        lib.pfnGetSystemCpuSetInformation
    );

    CHECK(result != nullptr);
}

TEST_CASE("Processor Geometry") {
    threads::init();
}

#if 0
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
        (void)threads::detail::buildCpuGeometry(temp);
    }
}
#endif
