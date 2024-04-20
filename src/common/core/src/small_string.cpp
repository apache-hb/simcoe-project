#include "core/small_string.hpp"
#include "core/units.hpp"

#include <cstring>

using namespace sm;

void SmallStringBase::init(char *buffer, const char *str, int16 length) {
    mLength = length;
    std::memcpy(buffer, str, mLength);
    buffer[mLength] = '\0';
}

void SmallStringBase::init(char *buffer, const char *str) {
    init(buffer, str, int_cast<int16>(std::strlen(str)));
}
