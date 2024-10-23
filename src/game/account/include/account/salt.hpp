#pragma once

#include <random>
#include <span>

namespace game {
    class Salt {
        std::mt19937 mSource;

        void newSalt(std::span<char> buffer);

    public:
        Salt();
        Salt(unsigned seed);

        std::string getSaltString(size_t length);
    };

    uint64_t hashWithSalt(std::string_view password, std::string_view salt);
}