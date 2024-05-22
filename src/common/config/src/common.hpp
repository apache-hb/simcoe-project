#pragma once

#include "config/option.hpp"

#include <fmtlib/format.h>

template<typename T>
constexpr bool inRange(sm::config::Range<T> range, T value) {
    return value >= range.min && value <= range.max;
}
