#include "core/bitmap.hpp"

using namespace sm;

// AtomicBitMap

bool AtomicBitMap::test_set(Index index) {
    auto mask = get_mask(index);

    return !(m_data[get_word(index)].fetch_or(mask) & mask);
}
