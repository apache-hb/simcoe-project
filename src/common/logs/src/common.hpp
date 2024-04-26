#pragma once

#include "core/span.hpp"
#include "logs/logs.hpp"

#include <experimental/generator>

namespace sm::logs {
    CT_LOCAL size_t buildMessageHeader(sm::Span<char> buffer, const Message& message) noexcept;
    CT_LOCAL size_t buildMessageHeaderWithColour(sm::Span<char> buffer, const Message& message, const colour_pallete_t& pallete) noexcept;

    CT_LOCAL std::experimental::generator<sm::StringView> splitMessage(sm::StringView message);
}
