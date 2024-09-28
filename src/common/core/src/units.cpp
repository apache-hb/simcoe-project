#include "stdafx.hpp"

#include "core/memory.hpp"

using namespace sm;

Memory::String Memory::toString() const noexcept {
    if (mBytes == 0) { return "0b"; }

    sm::FormatBuffer buffer;
    auto out = std::back_inserter(buffer);
    size_t total = mBytes;

    // seperate each part with a +

    for (int fmt = eCount - 1; fmt >= 0; fmt--) {
        size_t size = total / kSizes[fmt];
        if (size > 0) {
            fmt::format_to(out, "{}{}", size, kNames[fmt]);
            total %= kSizes[fmt];

            if (total > 0) {
                fmt::format_to(out, "+");
            }
        }
    }

    return String(buffer.data(), buffer.size());
}
