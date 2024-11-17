#pragma once

#include "core/fs.hpp"

namespace sm::system::storage {
    bool isSetup() noexcept;
    void create(void);
    void destroy(void) noexcept;

    /// @brief Get the path to the roaming configuration folder.
    /// This folder is intended for user-specific configuration
    /// data that should follow the user between different machines.
    /// i.e. hotkeys, save games, colour schemes, etc.
    fs::path getRoamingDataFolder();

    /// @brief Get the path to the local configuration folder.
    /// This folder is intended for user-specific configuration
    /// data that is specific to this machine.
    /// i.e. window positions
    fs::path getLocalDataFolder();

    /// @brief Get the path to the shared configuration folder.
    /// This folder is intended for machine specific configuration
    /// data that is shared between all users on this machine.
    /// i.e. PSO cache
    fs::path getMachineDataFolder();

    /// @brief Get the path to the static data folder.
    /// This folder contains all data included with the installation
    /// of this program, and is readonly.
    fs::path getProgramDataFolder();
}
