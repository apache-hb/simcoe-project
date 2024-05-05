#include "stdafx.hpp"

#include "core/bitmap.hpp"

using namespace sm;

// AtomicBitMap

bool AtomicBitMap::test_set(Index index) noexcept {
    auto mask = get_mask(index);

    return !(mStorage[get_word(index)].fetch_or(mask) & mask);
}
