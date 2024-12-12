#pragma once

#include <stdint.h>
#include <cstring>

#include <utility>
#include <functional>

namespace game {
    struct Guid {
        uint8_t data[16];

        static Guid empty() noexcept {
            return Guid{};
        }

        constexpr bool operator==(const Guid& other) const noexcept {
            return std::memcmp(data, other.data, sizeof(data)) == 0;
        }
    };
}

template<>
struct std::hash<game::Guid> {
    size_t operator()(const game::Guid& guid) const noexcept {
        size_t hash = 0;
        for (uint8_t byte : guid.data) {
            hash = hash * 31 + byte;
        }

        return hash;
    }
};
