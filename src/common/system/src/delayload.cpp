#include "core/win32.hpp" // IWYU pragma: export

#include "logs/logs.hpp"
#include "core/format.hpp" // IWYU pragma: export
#include "system/system.hpp"

#include "fmt/std.h" // IWYU pragma: export

#include "delayimp.h"

#include "delayload.reflect.h"

using namespace sm;

static FARPROC WINAPI dllDelayLoadHook(unsigned notify, PDelayLoadInfo info) {
    delayload::LoadNotify it{notify};

    if (it == delayload::LoadNotify::eFailLoadLibrary) {
        auto path = sm::sys::getProgramFolder() / "redist" / info->szDll;

        auto library = LoadLibrary(path.string().c_str());

        if (library != nullptr) {
            logs::gDebug.info("delay_hook: loaded: {}", path);
            return reinterpret_cast<FARPROC>(library);
        }

        logs::gDebug.warn("delay_hook: failed to load: {}", path);
    }

    return nullptr;
}

ExternC const PfnDliHook __pfnDliNotifyHook2 = dllDelayLoadHook;
ExternC const PfnDliHook __pfnDliFailureHook2 = dllDelayLoadHook;
