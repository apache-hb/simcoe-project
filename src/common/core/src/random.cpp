#include "core/random.hpp"

using namespace sm;

Random::Random()
    : Random(std::random_device{}())
{ }

Random::Random(unsigned seed)
    : mSource(seed)
{ }

void Random::nextBytes(std::span<uint8_t> buffer) noexcept {
    std::uniform_int_distribution<short> dist(0, std::numeric_limits<uint8_t>::max());
    for (uint8_t& byte : buffer) {
        byte = dist(mSource) & UCHAR_MAX;
    }
}
