#include "core/win32.hpp" // IWYU pragma: export
#include "logs/logs.hpp"
#include "core/format.hpp" // IWYU pragma: export
#include "system/system.hpp"

#include "fmt/std.h" // IWYU pragma: export

#include "delayimp.h"

#include "delayload.reflect.h"

using namespace sm;

static auto gSink = logs::get_sink(logs::Category::eDebug);

static FARPROC WINAPI delay_hook(unsigned notify, PDelayLoadInfo info) {
    delayload::LoadNotify it{notify};

    if (it == delayload::LoadNotify::eFailLoadLibrary) {
        auto path = sys::get_redist(info->szDll);

        auto library = LoadLibrary(path.string().c_str());

        if (library != nullptr) {
            gSink.info("delay_hook: loaded: {}", path);
            return reinterpret_cast<FARPROC>(library);
        }

        gSink.warn("delay_hook: failed to load: {}", path);
    }

    return nullptr;
}

ExternC const PfnDliHook __pfnDliNotifyHook2 = delay_hook;
ExternC const PfnDliHook __pfnDliFailureHook2 = delay_hook;
