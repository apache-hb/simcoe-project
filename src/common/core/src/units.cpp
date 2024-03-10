#include "core/memory.hpp"
#include "core/format.hpp"
#include "core/string.hpp"

#include <iterator>

using namespace sm;

sm::String Memory::to_string() const {
    if (mBytes == 0) { return "0b"; }

    sm::FormatBuffer buffer;
    auto out = std::back_inserter(buffer);
    size_t total = mBytes;

    // seperate each part with a +

    for (int fmt = eLimit - 1; fmt >= 0; fmt--) {
        size_t size = total / kSizes[fmt];
        if (size > 0) {
            fmt::format_to(out, "{}{}", size, kNames[fmt]);
            total %= kSizes[fmt];

            if (total > 0) {
                fmt::format_to(out, "+");
            }
        }
    }

    return sm::String(buffer.data(), buffer.size());
}
