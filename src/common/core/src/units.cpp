#include "core/units.hpp"

#include <vector>

using namespace sm;

static std::string join(const std::vector<std::string> &parts, const std::string &sep) {
    std::string result;

    for (size_t i = 0; i < parts.size(); i++) {
        if (i > 0) { result += sep; }
        result += parts[i];
    }

    return result;
}

std::string Memory::to_string() const {
    if (m_bytes == 0) { return "0b"; }

    std::vector<std::string> parts;
    size_t total = m_bytes;

    for (int fmt = eLimit - 1; fmt >= 0; fmt--) {
        size_t size = total / kSizes[fmt];
        if (size > 0) {
            parts.push_back(std::to_string(size) + kNames[fmt]); // TODO: we need our own string and string_view, this is ridiculous
            total %= kSizes[fmt];
        }
    }

    return ::join(parts, "+");
}
