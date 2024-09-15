#pragma once

#include <stdlib.h>

namespace sm::utf8 {
    // utf8 codepoint iterator, doesnt handle invalid utf8, surrogates, etc
    class TextIterator {
        const char8_t *mText;
        size_t mOffset;

    public:
        using difference_type = ptrdiff_t; // NOLINT
        using reference = char32_t; // NOLINT
        using value_type = char32_t; // NOLINT
        using pointer = const char32_t*; // NOLINT

        TextIterator(const char8_t *text, size_t offset);

        bool operator==(const TextIterator& other) const;
        bool operator!=(const TextIterator& other) const;

        TextIterator& operator++();

        char32_t operator*() const;
    };

    /// validate a utf8 string
    /// @return the offset of the first invalid codepoint, or SIZE_MAX if valid
    size_t validate(const char8_t *text, size_t length);

    // static utf8 string
    class StaticText {
        const char8_t *mText;
        size_t mSize;

    public:
        StaticText(const char8_t *text);

        StaticText(const char8_t *text, size_t size);

        const char8_t *data() const;
        size_t size_bytes() const;

        TextIterator begin() const;
        TextIterator end() const;
    };

    // dynamic utf8 string
    class String {
        char8_t *mFront;
        char8_t *mBack;
        char8_t *mCapacity;

    public:
        String();
        String(const char8_t *text);

        const char8_t *data() const;
        size_t size() const;

        TextIterator begin() const;
        TextIterator end() const;
    };
}
