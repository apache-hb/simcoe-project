#include "core/bitmap.hpp"

using namespace sm;

// AtomicBitMap

bool AtomicBitMap::test_set(size_t index) {
    auto mask = get_mask(index);

    return !(m_bits[get_word(index)].fetch_or(mask) & mask);
}
