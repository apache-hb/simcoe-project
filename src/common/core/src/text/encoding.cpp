#include "core/text/encoding.hpp"

void sm::text::asciiToPetscii(std::string& text) {
    for (char& c : text) {
        if (c >= 0x20 && c <= 0x5f) {
            c += 0x80;
        } else if (c >= 0x60 && c <= 0x7f) {
            c -= 0x20;
        }
    }
}
