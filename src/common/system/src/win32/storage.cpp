#include "system/storage.hpp"

#include <simcoe_config.h>

#include "base/panic.h"

#include "core/error.hpp"

#include "core/win32.hpp"
#include <shlobj_core.h>

#include <wil/com.h>

using namespace sm;
using namespace sm::system;

static bool gStorageSetup = false;
static fs::path gRoamingDataFolder;
static fs::path gLocalDataFolder;
static fs::path gMachineDataFolder;
static fs::path gProgramDataFolder;

static fs::path getKnownFolderPath(REFKNOWNFOLDERID rfid) {
    wil::unique_cotaskmem_string str;
    HRESULT hr = SHGetKnownFolderPath(rfid, KF_FLAG_DEFAULT, nullptr, &str);
    if (FAILED(hr)) {
        throw OsException{OsError(hr), "SHGetKnownFolderPath"};
    }

    return fs::path{str.get()};
}

bool storage::isSetup() noexcept {
    return gStorageSetup;
}

void storage::create(void) {
    CTASSERTF(!isSetup(), "storage already initialized");

    gRoamingDataFolder = getKnownFolderPath(FOLDERID_RoamingAppData) / SMC_ENGINE_NAME;
    gLocalDataFolder = getKnownFolderPath(FOLDERID_LocalAppData) / SMC_ENGINE_NAME;
    gMachineDataFolder = getKnownFolderPath(FOLDERID_ProgramData) / SMC_ENGINE_NAME;
    gProgramDataFolder = getKnownFolderPath(FOLDERID_ProgramFiles) / SMC_ENGINE_NAME;

    gStorageSetup = true;
}

void storage::destroy(void) noexcept {
    if (!isSetup())
        return;

    gStorageSetup = false;
}

fs::path storage::getRoamingDataFolder() {
    return gRoamingDataFolder;
}

fs::path storage::getLocalDataFolder() {
    return gLocalDataFolder;
}

fs::path storage::getMachineDataFolder() {
    return gMachineDataFolder;
}

fs::path storage::getProgramDataFolder() {
    return fs::path{};
}
