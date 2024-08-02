#include "stdafx.hpp"

#include "render/vendor/nvidia/nvapi.hpp"

#include <nvapi.h>

using namespace sm;
using namespace sm::render;

NvString NvStatus::message() const {
    NvAPI_ShortString buffer;
    NvAPI_GetErrorMessage(NvAPI_Status(mStatus), buffer);
    return NvString(buffer);
}

NvResult<NvString> nvapi::getInterfaceVersionString() {
    NvAPI_ShortString buffer;
    auto status = NvAPI_GetInterfaceVersionString(buffer);
    return NvResult(NvStatus(status), NvString(buffer));
}

NvResult<NvString> nvapi::getInterfaceVersionStringEx() {
    NvAPI_ShortString buffer;
    auto status = NvAPI_GetInterfaceVersionStringEx(buffer);
    return NvResult(NvStatus(status), NvString(buffer));
}

NvResult<NvVersion> nvapi::getDriverVersion() {
    NvU32 version;
    NvAPI_ShortString build;

    NvStatus status = NvAPI_SYS_GetDriverAndBranchVersion(&version, build);
    if (!status.success()) {
        return NvResult(status, NvVersion{});
    }

    return NvResult(status, NvVersion{version, NvString(build)});
}

NvStatus nvapi::startup() {
    return NvAPI_Initialize();
}

NvStatus nvapi::shutdown() {
    return NvAPI_Unload();
}
