#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::logs;

static constexpr colour_t getSeverityColour(Severity severity) {
    using enum logs::Severity;
    switch (severity) {
    case eTrace: return eColourWhite;
    case eInfo: return eColourGreen;
    case eWarning: return eColourYellow;
    case eError: return eColourRed;
    case ePanic: return eColourMagenta;
    default: return eColourDefault;
    }
}

struct TimeUnits {
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;
    uint32_t milliseconds;
};

static constexpr TimeUnits getTimeUnits(uint32_t timestamp) {
    return TimeUnits {
        .hours = (timestamp / (60 * 60 * 1000)) % 24,
        .minutes = (timestamp / (60 * 1000)) % 60,
        .seconds = (timestamp / 1000) % 60,
        .milliseconds = timestamp % 1000,
    };
}

static size_t buildMessageInner(sm::Span<char> buffer, uint32_t timestamp, const structured::MessageInfo& message, const char *colour, const char *reset) {
    auto [h, m, s, ms] = getTimeUnits(timestamp);

    return fmt::format_to_n(buffer.data(), buffer.size(), "{}[{}]{}[{:02}:{:02}:{:02}.{:03}] {}:", colour, message.level, reset, h, m, s, ms, message.category.name).size;
}

size_t logs::detail::buildMessageHeader(sm::Span<char> buffer, uint32_t timestamp, const structured::MessageInfo& message) noexcept {
    return buildMessageInner(buffer, timestamp, message, "", "");
}

size_t logs::detail::buildMessageHeaderWithColour(sm::Span<char> buffer, uint32_t timestamp, const structured::MessageInfo& message, const colour_pallete_t& pallete) noexcept {
    const char *colour = colour_get(&pallete, getSeverityColour(message.level));
    const char *reset = colour_reset(&pallete);

    return buildMessageInner(buffer, timestamp, message, colour, reset);
}
