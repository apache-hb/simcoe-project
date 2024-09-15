#include "stdafx.hpp"

#include "core/utf8.hpp"
#include "base/panic.h"

#include <stdint.h>

using namespace sm;
using namespace sm::utf8;

/// get the length of a utf8 string in bytes
static constexpr size_t utf8_size_bytes(const char8_t *text) {
    size_t length = 0;
    while (*text++) {
        length += 1;
    }
    return length;
}

/// get the length of a single codepoint in bytes
static constexpr size_t utf8_codepoint_length(const char8_t *text) {
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

size_t sm::utf8::validate(const char8_t *text, size_t length) {
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

        size_t size = utf8_codepoint_length(text + offset);
        if (size == 0) {
            return offset;
        }
        offset += size;
    }
    return SIZE_MAX;
}

// text iterator

TextIterator::TextIterator(const char8_t *text, size_t offset)
    : mText(text)
    , mOffset(offset)
{ }

bool TextIterator::operator==(const TextIterator& other) const {
    return mText == other.mText && mOffset == other.mOffset;
}

bool TextIterator::operator!=(const TextIterator& other) const {
    return mText != other.mText || mOffset != other.mOffset;
}

TextIterator& TextIterator::operator++() {
    if ((mText[mOffset] & 0x80) == 0) {
        mOffset += 1;
    } else if ((mText[mOffset] & 0xE0) == 0xC0) {
        mOffset += 2;
    } else if ((mText[mOffset] & 0xF0) == 0xE0) {
        mOffset += 3;
    } else if ((mText[mOffset] & 0xF8) == 0xF0) {
        mOffset += 4;
    }

    return *this;
}

char32_t TextIterator::operator*() const {
    char32_t codepoint = 0;

    if ((mText[mOffset] & 0x80) == 0) {
        codepoint = mText[mOffset];
    } else if ((mText[mOffset] & 0xE0) == 0xC0) {
        codepoint = (mText[mOffset] & 0x1F) << 6;
        codepoint |= (mText[mOffset + 1] & 0x3F);
    } else if ((mText[mOffset] & 0xF0) == 0xE0) {
        codepoint = (mText[mOffset] & 0x0F) << 12;
        codepoint |= (mText[mOffset + 1] & 0x3F) << 6;
        codepoint |= (mText[mOffset + 2] & 0x3F);
    } else if ((mText[mOffset] & 0xF8) == 0xF0) {
        codepoint = (mText[mOffset] & 0x07) << 18;
        codepoint |= (mText[mOffset + 1] & 0x3F) << 12;
        codepoint |= (mText[mOffset + 2] & 0x3F) << 6;
        codepoint |= (mText[mOffset + 3] & 0x3F);
    }

    return codepoint;
}

// static text

StaticText::StaticText(const char8_t *text)
    : mText(text)
    , mSize(utf8_size_bytes(text))
{
#if SMC_DEBUG
    size_t offset = sm::utf8::validate(text, mSize);
    CTASSERTF(offset == SIZE_MAX, "invalid utf8 text at offset %zu", offset);
#endif
}

StaticText::StaticText(const char8_t *text, size_t size)
    : mText(text)
    , mSize(size)
{ }

const char8_t *StaticText::data() const { return mText; }
size_t StaticText::size_bytes() const { return mSize; }

TextIterator StaticText::begin() const {
    return TextIterator(mText, 0);
}

TextIterator StaticText::end() const {
    return TextIterator(mText, mSize);
}
