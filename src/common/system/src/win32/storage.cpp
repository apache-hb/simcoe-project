#include "system/storage.hpp"

#include "base/panic.h"

#include "core/win32.hpp"

using namespace sm;
using namespace sm::system;

static bool gStorageSetup = false;

bool storage::isSetup() noexcept {
    return gStorageSetup;
}

void storage::create(void) {
    CTASSERTF(!isSetup(), "storage already initialized");

    gStorageSetup = true;
}

void storage::destroy(void) noexcept {
    if (!isSetup())
        return;

    gStorageSetup = false;
}

fs::path storage::getRoamingConfig() {

}

fs::path storage::getLocalConfig() {

}

fs::path storage::getMachineConfig() {

}
