#include "system/storage.hpp"

#include <simcoe_config.h>

#include "base/panic.h"

#include "core/error.hpp"

#include "core/win32.hpp"
#include "system/system.hpp"
#include <shlobj_core.h>

#include <wil/com.h>

using namespace sm;
using namespace sm::system;

static fs::path gRoamingDataFolder;
static fs::path gLocalDataFolder;
static fs::path gMachineDataFolder;
static fs::path gProgramDataFolder;
static fs::path gProgramPath;
static fs::path gProgramFolder;
static std::string gProgramName;

static fs::path getKnownFolderPath(REFKNOWNFOLDERID rfid) {
    wil::unique_cotaskmem_string str;
    HRESULT hr = SHGetKnownFolderPath(rfid, KF_FLAG_DEFAULT, nullptr, &str);
    if (FAILED(hr)) {
        throw OsException{OsError(hr), "SHGetKnownFolderPath"};
    }

    return fs::path{str.get()};
}

void initStorage() {
    gRoamingDataFolder = getKnownFolderPath(FOLDERID_RoamingAppData) / SMC_ENGINE_NAME;
    gLocalDataFolder = getKnownFolderPath(FOLDERID_LocalAppData) / SMC_ENGINE_NAME;
    gMachineDataFolder = getKnownFolderPath(FOLDERID_ProgramData) / SMC_ENGINE_NAME;
    gProgramDataFolder = getKnownFolderPath(FOLDERID_ProgramFiles) / SMC_ENGINE_NAME;

    static constexpr size_t kPathMax = 0x1000;
    TCHAR gExecutablePath[kPathMax];
    DWORD gExecutablePathLength = 0;

    gExecutablePathLength = GetModuleFileNameA(nullptr, gExecutablePath, kPathMax);

    if (gExecutablePathLength == 0) {
        assertLastError(CT_SOURCE_CURRENT, "GetModuleFileNameA");
    }

    if (gExecutablePathLength >= kPathMax) {
        LOG_WARN(SystemLog, "Executable path longer than {}, truncating.", kPathMax);
    }

    gProgramPath = fs::path{gExecutablePath, gExecutablePath + gExecutablePathLength};
    gProgramFolder = gProgramPath.parent_path();
    gProgramName = gProgramPath.stem().string();
}

fs::path system::getRoamingDataFolder() {
    return gRoamingDataFolder;
}

fs::path system::getLocalDataFolder() {
    return gLocalDataFolder;
}

fs::path system::getMachineDataFolder() {
    return gMachineDataFolder;
}

fs::path system::getProgramDataFolder() {
    return gProgramFolder; // TODO: this depends on how the program is installed
}

fs::path system::getProgramFolder() {
    return gProgramFolder;
}

fs::path system::getProgramPath() {
    return gProgramPath;
}

std::string system::getProgramName() {
    return gProgramName;
}
