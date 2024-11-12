#include "system/storage.hpp"

#include "base/panic.h"

#include "core/win32.hpp"

#include <wil/com.h>

using namespace sm;
using namespace sm::system;

static bool gStorageSetup = false;
static wil::unique_couninitialize_call gCoUninitialize;

bool storage::isSetup() noexcept {
    return gStorageSetup;
}

void storage::create(void) {
    CTASSERTF(!isSetup(), "storage already initialized");

    gStorageSetup = true;

    gCoUninitialize = wil::CoInitializeEx(COINIT_MULTITHREADED);
}

void storage::destroy(void) noexcept {
    if (!isSetup())
        return;

    gStorageSetup = false;
    gCoUninitialize.reset();
}
