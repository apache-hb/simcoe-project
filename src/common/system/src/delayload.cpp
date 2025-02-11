#include "core/win32.hpp" // IWYU pragma: export

#include "core/format.hpp" // IWYU pragma: export

#include "system/system.hpp"
#include "system/storage.hpp"

#include "fmt/std.h" // IWYU pragma: export

#include <delayimp.h>

using namespace sm;

static FARPROC WINAPI dllDelayLoadHook(unsigned notify, PDelayLoadInfo info) {
    if (notify == dliFailLoadLib) {
        fs::path path = sm::system::getProgramFolder() / "redist" / info->szDll;

        HMODULE library = LoadLibrary(path.string().c_str());

        if (library != nullptr) {
            LOG_INFO(SystemLog, "dllDelayLoadHook: loaded: {}", path);
            return reinterpret_cast<FARPROC>(library);
        }

        LOG_WARN(SystemLog, "dllDelayLoadHook: failed to load: {}", path);
    }

    return nullptr;
}

ExternC const PfnDliHook __pfnDliNotifyHook2 = dllDelayLoadHook;
ExternC const PfnDliHook __pfnDliFailureHook2 = dllDelayLoadHook;
