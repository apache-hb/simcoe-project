#pragma once

#ifdef _WIN32
#   include "core/win32.hpp" // IWYU pragma: export
#endif

#include "core/fs.hpp"

namespace sm {
    bool pix_enabled();
    bool warp_enabled();
    bool dred_enabled();
    bool debug_enabled();
    fs::path get_appdir();
    fs::path get_redist(const fs::path& path);

#ifdef _WIN32
    std::optional<LUID> override_adapter_luid();
#endif

    bool parse_command_line(int argc, const char **argv, const fs::path& appdir);

    int parseCommandLine(int argc, const char **argv, const fs::path& appdir);
}
