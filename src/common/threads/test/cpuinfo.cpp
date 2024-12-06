#include <gtest/gtest.h>

#include "threads/threads.hpp"

#include "cpuinfo.hpp"

using namespace sm;

using CpuInfoLibrary = threads::detail::CpuInfoLibrary;

TEST(GeometryTest, CpuSet) {
    CpuInfoLibrary lib = CpuInfoLibrary::load();
    if (lib.pfnGetSystemCpuSetInformation == nullptr)
        GTEST_SKIP() << "GetSystemCpuSetInformation not available";

    auto cpuSetInfo = threads::detail::readSystemCpuSetInformation(lib.pfnGetSystemCpuSetInformation);
    ASSERT_NE(cpuSetInfo.data, nullptr);
    ASSERT_NE(cpuSetInfo.size, 0);
}

TEST(GeometryTest, ProcessorInfo) {
    CpuInfoLibrary lib = CpuInfoLibrary::load();
    if (lib.pfnGetLogicalProcessorInformationEx == nullptr)
        GTEST_SKIP() << "GetLogicalProcessorInformationEx not available";

    auto cpuSetInfo = threads::detail::readLogicalProcessorInformationEx(lib.pfnGetLogicalProcessorInformationEx, RelationAll);
    ASSERT_NE(cpuSetInfo.data, nullptr);
    ASSERT_NE(cpuSetInfo.size, 0);
}

TEST(GeometryTest, BuildGeometry) {
    threads::create();
    threads::destroy();
}
