#include "stdafx.hpp"

#include "render/vendor/nvidia/nvapi.hpp"

#include <nvapi.h>

using namespace sm;
using namespace sm::render;

static_assert(NvString::kCapacity >= NVAPI_SHORT_STRING_MAX);

NvString NvStatus::message() const {
    NvAPI_ShortString buffer;
    NvAPI_GetErrorMessage(NvAPI_Status(mStatus), buffer);
    return NvString(buffer);
}

NvResult<NvString> nvapi::getInterfaceVersionString() {
    NvAPI_ShortString buffer;
    NvStatus status = NvAPI_GetInterfaceVersionString(buffer);
    if (!status.success())
        return std::unexpected(status);

    return NvString(buffer);
}

NvResult<NvString> nvapi::getInterfaceVersionStringEx() {
    NvAPI_ShortString buffer;
    NvStatus status = NvAPI_GetInterfaceVersionStringEx(buffer);
    if (!status.success())
        return std::unexpected(status);

    return NvString(buffer);
}

NvResult<NvVersion> nvapi::getDriverVersion() {
    NvU32 version;
    NvAPI_ShortString build;

    NvStatus status = NvAPI_SYS_GetDriverAndBranchVersion(&version, build);
    if (!status.success())
        return std::unexpected(status);

    return NvVersion{version, NvString(build)};
}

NvStatus nvapi::create(void) {
    return NvAPI_Initialize();
}

NvStatus nvapi::destroy(void) noexcept {
    return NvAPI_Unload();
}
