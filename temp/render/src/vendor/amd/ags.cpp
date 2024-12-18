#include "stdafx.hpp"

#include "render/vendor/amd/ags.hpp"

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wignored-attributes"
#endif

#include <amd_ags.h>

#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

using namespace sm;
using namespace sm::render;

static_assert(sizeof(AGSReturnCode) <= sizeof(int), "AGSReturnCode size mismatch");

LOG_MESSAGE_CATEGORY(AgsLog, "AMD Gpu Services");

static AGSContext *gAgsContext = nullptr;
static AGSGPUInfo gAgsGpuInfo = {};

static void *agsMalloc(size_t size) {
    return ::malloc(size);
}

static void agsFree(void *ptr) {
    ::free(ptr);
}

#define ENUM_CASE(x) case x: return #x

AgsString AgsError::message() const {
    switch (mStatus) {
    ENUM_CASE(AGS_SUCCESS);
    ENUM_CASE(AGS_FAILURE);
    ENUM_CASE(AGS_OUT_OF_MEMORY);
    ENUM_CASE(AGS_MISSING_D3D_DLL);
    ENUM_CASE(AGS_LEGACY_DRIVER);
    ENUM_CASE(AGS_NO_AMD_DRIVER_INSTALLED);
    ENUM_CASE(AGS_EXTENSION_NOT_SUPPORTED);
    ENUM_CASE(AGS_ADL_FAILURE);
    ENUM_CASE(AGS_DX_FAILURE);
    ENUM_CASE(AGS_D3DDEVICE_NOT_CREATED);

    default: return fmt::format("AGS_UNKNOWN({:#X})", mStatus);
    }
}

AgsError ags::create(void) {
    AGSConfiguration config = {
        .allocCallback = agsMalloc,
        .freeCallback = agsFree,
    };

    return agsInitialize(AGS_CURRENT_VERSION, &config, &gAgsContext, &gAgsGpuInfo);
}

AgsError ags::destroy(void) noexcept {
    return agsDeInitialize(gAgsContext);
}

AgsVersion ags::getVersion() {
    int version = agsGetVersionNumber();

    int major = (version >> 24) & 0xFF;
    int minor = (version >> 16) & 0xFF;
    int patch = version & 0xFFFF;

    return AgsVersion {
        .driver = gAgsGpuInfo.driverVersion,
        .runtime = gAgsGpuInfo.radeonSoftwareVersion,
        .major = major,
        .minor = minor,
        .patch = patch
    };
}
