#pragma once

#include "core/win32.hpp" // IWYU pragma: export
#include "core/fs.hpp"

namespace sm {
    bool pix_enabled();
    bool warp_enabled();
    bool dred_enabled();
    bool debug_enabled();
    fs::path get_appdir();
    fs::path get_redist(const fs::path& path);

    std::optional<LUID> override_adapter_luid();

    bool parse_command_line(int argc, const char **argv, const fs::path& appdir);
}
