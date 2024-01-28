#include "core/utf8.hpp"
#include "base/panic.h"

#include <stdint.h>

using namespace sm;
using namespace sm::utf8;

namespace {
    /// get the length of a utf8 string in bytes
    constexpr size_t utf8_string_length(const char8_t *text) {
        size_t length = 0;
        while (*text++) {
            length += 1;
        }
        return length;
    }

    /// get the length of a single codepoint in bytes
    constexpr size_t utf8_codepoint_length(const char8_t *text) {
        if ((text[0] & 0x80) == 0) {
            return 1;
        } else if ((text[0] & 0xE0) == 0xC0) {
            return 2;
        } else if ((text[0] & 0xF0) == 0xE0) {
            return 3;
        } else if ((text[0] & 0xF8) == 0xF0) {
            return 4;
        } else {
            return 0;
        }
    }

    /// validate a utf8 string
    /// @return the offset of the first invalid codepoint, or SIZE_MAX if valid
    constexpr size_t utf8_validate(const char8_t *text, size_t length) {
        size_t offset = 0;
        while (offset < length) {
            // check for invalid bytes
            char8_t byte = text[offset];
            switch (byte) {
            case 0xFE:
            case 0xFF:
                return offset;
            default:
                break;
            }

            size_t codepointSize = utf8_codepoint_length(text + offset);
            if (codepointSize == 0) {
                return offset;
            }
            offset += codepointSize;
        }
        return SIZE_MAX;
    }
}

// text iterator

TextIterator::TextIterator(const char8_t *text, size_t offset)
    : m_text(text)
    , m_offset(offset)
{ }

bool TextIterator::operator==(const TextIterator& other) const {
    return m_text == other.m_text && m_offset == other.m_offset;
}

bool TextIterator::operator!=(const TextIterator& other) const {
    return m_text != other.m_text || m_offset != other.m_offset;
}

TextIterator& TextIterator::operator++() {
    if ((m_text[m_offset] & 0x80) == 0) {
        m_offset += 1;
    } else if ((m_text[m_offset] & 0xE0) == 0xC0) {
        m_offset += 2;
    } else if ((m_text[m_offset] & 0xF0) == 0xE0) {
        m_offset += 3;
    } else if ((m_text[m_offset] & 0xF8) == 0xF0) {
        m_offset += 4;
    }

    return *this;
}

char32_t TextIterator::operator*() const {
    char32_t codepoint = 0;

    if ((m_text[m_offset] & 0x80) == 0) {
        codepoint = m_text[m_offset];
    } else if ((m_text[m_offset] & 0xE0) == 0xC0) {
        codepoint = (m_text[m_offset] & 0x1F) << 6;
        codepoint |= (m_text[m_offset + 1] & 0x3F);
    } else if ((m_text[m_offset] & 0xF0) == 0xE0) {
        codepoint = (m_text[m_offset] & 0x0F) << 12;
        codepoint |= (m_text[m_offset + 1] & 0x3F) << 6;
        codepoint |= (m_text[m_offset + 2] & 0x3F);
    } else if ((m_text[m_offset] & 0xF8) == 0xF0) {
        codepoint = (m_text[m_offset] & 0x07) << 18;
        codepoint |= (m_text[m_offset + 1] & 0x3F) << 12;
        codepoint |= (m_text[m_offset + 2] & 0x3F) << 6;
        codepoint |= (m_text[m_offset + 3] & 0x3F);
    }

    return codepoint;
}

// static text

StaticText::StaticText(const char8_t *text)
    : m_text(text)
    , m_size(utf8_string_length(text))
{
    size_t offset = utf8_validate(text, m_size);
    CTASSERTF(offset != SIZE_MAX, "invalid utf8 text at offset %zu", offset);
}

StaticText::StaticText(const char8_t *text, size_t size)
    : m_text(text)
    , m_size(size)
{ }

const char8_t *StaticText::data() const { return m_text; }
size_t StaticText::size() const { return m_size; }

TextIterator StaticText::begin() const {
    return TextIterator(m_text, 0);
}

TextIterator StaticText::end() const {
    return TextIterator(m_text, m_size);
}
