#include "stdafx.hpp"

#include "core/typeindex.hpp"

uint32_t sm::next_index() noexcept {
    static uint32_t index = 0;
    return index++;
}
