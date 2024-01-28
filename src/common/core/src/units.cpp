#include "core/units.hpp"
#include "core/text.hpp"

#include <iterator>

using namespace sm;

using FormatBuffer = fmt::basic_memory_buffer<char, 256, sm::StandardArena<char>>;

sm::String Memory::to_string() const {
    if (m_bytes == 0) { return "0b"; }

    FormatBuffer buffer;
    auto out = std::back_inserter(buffer);
    size_t total = m_bytes;

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

    return { buffer.data(), buffer.size() };
}
