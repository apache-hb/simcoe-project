#include "account/salt.hpp"

#include <fmtlib/format.h>

using namespace game;

static constexpr std::string_view kChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

Salt::Salt()
    : mSource{std::random_device{}()}
{ }

Salt::Salt(unsigned seed)
    : mSource{seed}
{ }

void Salt::newSalt(std::span<char> buffer) {
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

uint64_t game::hashWithSalt(std::string_view password, std::string_view salt) {
    std::string combined = fmt::format("{}{}", password, salt);
    return std::hash<std::string>{}(combined);
}
