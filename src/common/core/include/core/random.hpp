#pragma once

#include <random>
#include <span>

namespace sm {
    class Random {
        std::mt19937 mSource;

    public:
        Random();
        Random(unsigned seed);

        void nextBytes(std::span<uint8_t> buffer) noexcept;
    };
}
