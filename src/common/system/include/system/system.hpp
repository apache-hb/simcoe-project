#pragma once

#include <simcoe_config.h>

#include "core/source_info.h"

#include "core/error.hpp"
#include "logs/logs.hpp" // IWYU pragma: export

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

    std::string getMachineId();
    std::vector<std::string> getCommandLine();

    void create(HINSTANCE hInstance);
    void destroy(void) noexcept;
}
