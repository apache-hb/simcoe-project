#pragma once

#include <stdint.h>
#include <cstring>

#include <utility>
#include <functional>

namespace game {
    struct Guid {
        uint8_t data[16];

        static Guid empty() {
            return Guid{};
        }

        constexpr bool operator==(const Guid& other) const {
            return std::memcmp(data, other.data, sizeof(data)) == 0;
        }
    };
}

template<>
struct std::hash<game::Guid> {
    size_t operator()(const game::Guid& guid) const {
        size_t hash = 0;
        for (uint8_t byte : guid.data) {
            hash = hash * 31 + byte;
        }

        return hash;
    }
};
