#pragma once

#include <cstdlib>
#include <iterator>

namespace sm::utf8 {
    // utf8 codepoint iterator, doesnt handle invalid utf8, surrogates, etc
    class TextIterator {
        const char8_t *m_text;
        size_t m_offset;

    public:
        using difference_type = ptrdiff_t;
        using reference = char32_t;
        using value_type = char32_t;
        using pointer = const char32_t*;
        using iterator_category = std::forward_iterator_tag;

        TextIterator(const char8_t *text, size_t offset);

        bool operator==(const TextIterator& other) const;
        bool operator!=(const TextIterator& other) const;

        TextIterator& operator++();

        char32_t operator*() const;
    };

    // static utf8 string
    class StaticText {
        const char8_t *m_text;
        size_t m_size;

    public:
        StaticText(const char8_t *text);

        StaticText(const char8_t *text, size_t size);

        const char8_t *data() const;
        size_t size() const;

        TextIterator begin() const;
        TextIterator end() const;
    };

    // dynamic utf8 string
    class String {
        char8_t *m_front;
        char8_t *m_back;
        char8_t *m_capacity;

    public:
        String();
        String(const char8_t *text);

        const char8_t *data() const;
        size_t size() const;

        TextIterator begin() const;
        TextIterator end() const;
    };
}
