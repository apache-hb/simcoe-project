#pragma once

#include "core/span.hpp"
#include "logs/logs.hpp"

namespace sm::logs {
    CT_LOCAL size_t buildMessageHeader(sm::Span<char> buffer, const Message& message) noexcept;
    CT_LOCAL size_t buildMessageHeaderWithColour(sm::Span<char> buffer, const Message& message, const colour_pallete_t& pallete) noexcept;

    void splitMessage(sm::StringView message, auto fn) noexcept {
        size_t start = 0;
        size_t end = 0;
        while (end < message.size()) {
            if (message[end] == '\n') {
                fn(message.substr(start, end - start));
                start = end + 1;
            }
            end++;
        }

        if (start < end) {
            fn(message.substr(start, end - start));
        }
    }
}
