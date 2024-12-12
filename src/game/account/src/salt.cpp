#include "account/salt.hpp"

#include "core/hash.hpp"

#include <fmtlib/format.h>

using namespace game;

static constexpr std::string_view kChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

Salt::Salt()
    : mSource{std::random_device{}()}
{ }

Salt::Salt(unsigned seed)
    : mSource{seed}
{ }

void Salt::newSalt(std::span<char> buffer) noexcept {
    std::uniform_int_distribution<int> anySaltChar(0, kChars.size() - 1);

    for (char& c : buffer) {
        c = kChars[anySaltChar(mSource)];
    }
}

std::string Salt::getSaltString(size_t length) {
    std::string buffer(length, '\0');
    newSalt({ buffer.data(), length });
    return buffer;
}

Guid Salt::newGuid() noexcept {
    std::uniform_int_distribution<> anyByte(0, 255);
    Guid guid{};
    for (uint8_t& byte : guid.data) {
        byte = anyByte(mSource);
    }

    return guid;
}

uint64_t game::hashWithSalt(std::string_view password, std::string_view salt) noexcept {
    size_t hash = 0x9e3779b9uz;
    sm::hash_combine(hash, password, salt);
    return hash;
}
