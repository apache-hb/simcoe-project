#include "stdafx.hpp"

#include "render/core/vendor/amd/ags.hpp"

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

static LOG_CATEGORY_IMPL(gAgsLogs, "ags");

static AGSContext *gAgsContext = nullptr;
static AGSGPUInfo gAgsGpuInfo = {};

template<typename T>
static T getLibrarySymbol(HMODULE module, const char *name) {
    auto symbol = (T)(void*)::GetProcAddress(module, name);
    if (!symbol) {
        gAgsLogs.error("Failed to get symbol: {}", name);
        return nullptr;
    }

    return symbol;
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

    default: return fmt::format("Unknown error: {}", mStatus);
    }
}

AgsError ags::startup() {
    AGSConfiguration config = {
        .allocCallback = malloc,
        .freeCallback = free,
    };

    return agsInitialize(AGS_CURRENT_VERSION, &config, &gAgsContext, &gAgsGpuInfo);
}

AgsError ags::shutdown() {
    return agsDeInitialize(gAgsContext);
}

AgsVersion ags::getVersion() {
    int version = agsGetVersionNumber();

    int major = (version >> 24) & 0xFF;
    int minor = (version >> 16) & 0xFF;
    int patch = version & 0xFFFF;

    return AgsVersion{ major, minor, patch };
}
