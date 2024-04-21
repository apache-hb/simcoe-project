#include "core/typeindex.hpp"

sm::uint32 sm::next_index() noexcept {
    static uint32 index = 0;
    return index++;
}
