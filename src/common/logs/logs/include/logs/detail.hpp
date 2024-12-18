#pragma once

#include <string_view>
#include <stdint.h>

namespace sm::logs::detail {
    consteval uint64_t hashMessage(std::string_view message) noexcept {
        uint64_t hash = 0x811c9dc5;
        for (char c : message) {
            hash = (hash * 31) + c;
        }
        return hash;
    }
}
