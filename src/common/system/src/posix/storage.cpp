#include "system/storage.hpp"

#include <simcoe_config.h>

#include "base/panic.h"

#include "core/error.hpp"

#include "system/system.hpp"

#include <sys/types.h>
#include <pwd.h>

using namespace sm;
using namespace sm::system;

static fs::path gRoamingDataFolder;
static fs::path gLocalDataFolder;
static fs::path gMachineDataFolder;
static fs::path gProgramDataFolder;
static fs::path gHomeFolder;
static fs::path gProgramPath;
static fs::path gProgramFolder;
static std::string gProgramName;

static std::optional<fs::path> getXdgPath(const char *name) {
    if (const char *path = getenv(name)) {
        return path;
    }

    return std::nullopt;
}

static fs::path getHome() {
    if (const char *home = getenv("HOME")) {
        return home;
    }

    size_t sz = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (sz == 0) {
        throw OsException{getLastError(), "sysconf(_SC_GETPW_R_SIZE_MAX)"};
    }

    if (sz == SIZE_MAX) sz = 0x1000;

    std::unique_ptr<char[]> buffer{new char[sz]};

    struct passwd pw{};
    struct passwd *result = nullptr;
    int err = getpwuid_r(getuid(), &pw, buffer.get(), sz, &result);
    if (result == nullptr) {
        if (err == 0) {
            throw OsException{OsError(ENOENT), "Current user does not exist. (how did we get here?)"};
        }

        throw OsException{getLastError(), "getpwuid_r"};
    }

    return pw.pw_dir;
}

static fs::path readCurrentPath() {
    const int kBufferSize = 0x1000;
    char buffer[kBufferSize] = {};
    int count = readlink("/proc/self/exe", buffer, sizeof(buffer));

    if (count >= (int)sizeof(buffer)) {
        throw OsException{OsError(ENAMETOOLONG), "Path to program is longer than {} chars. Please move me to somewhere with a shorter path.", sizeof(buffer)};
    }

    if (count < 0) {
        throw OsException{getLastError(), "readlink(\"/proc/self/exe\")"};
    }

    return std::string(buffer, count);
}

void initStorage() {
    fs::path home = getHome();
    fs::path self = readCurrentPath();

    gHomeFolder = home;

    gRoamingDataFolder = getXdgPath("XDG_DATA_HOME").value_or(home / ".local" / "share") / SMC_ENGINE_NAME;
    gLocalDataFolder = getXdgPath("XDG_CONFIG_HOME").value_or(home / ".config") / SMC_ENGINE_NAME;
    gMachineDataFolder = getXdgPath("XDG_CACHE_HOME").value_or(home / ".cache") / SMC_ENGINE_NAME;
    gProgramDataFolder = home.parent_path().parent_path() / "data";

    gProgramPath = self;
    gProgramFolder = self.parent_path();
    gProgramName = self.stem().string();
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

fs::path system::getHomeFolder() {
    return gHomeFolder;
}

std::string system::getProgramName() {
    return gProgramName;
}
