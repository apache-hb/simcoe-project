#pragma once

namespace sm::system::storage {
    bool isSetup() noexcept;
    void create(void);
    void destroy(void) noexcept;
}
