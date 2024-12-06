#pragma once

#include <simcoe_config.h>

#include "core/source_info.h"

#if CT_OS_WINDOWS
#   include "system/win32/system.hpp"
#elif CT_OS_LINUX
#   include "system/posix/system.hpp"
#endif

#include "core/error.hpp"

#include "logs/logging.hpp"

#if _WIN32
#   include "core/win32.hpp"
#else
typedef void *HINSTANCE;
#endif

LOG_MESSAGE_CATEGORY(SystemLog, "System");

namespace sm::system {
    CT_NORETURN assertLastError(source_info_t panic, const char *expr);

    OsError getLastError();

    os_error_t getMachineId(char buffer[kMachineIdSize]) noexcept;

    std::string getMachineId();
    std::vector<std::string> getCommandLine();

    void create(HINSTANCE hInstance);
    void destroy(void) noexcept;
}
