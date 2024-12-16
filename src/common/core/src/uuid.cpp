#include "core/uuid.hpp"

#include <fmtlib/format.h>
#include <fmt/chrono.h>

using namespace sm;

namespace chrono = std::chrono;

using namespace std::chrono_literals;

uuid uuid::v1(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node) {
    detail::rfc9562_clock::time_point clockOffset = chrono::clock_cast<detail::rfc9562_clock>(time);

    uint64_t intervals = clockOffset.time_since_epoch().count();

    uint16_t uuidClockSeq
        = (uuid::eRfc9562 << 14) // 64:65
        | (clockSeq & 0b0011'1111'1111'1111); // 66:79

    uuidv1 result = {
        .time0 = std::byteswap(intervals) >> 32,
        .time1 = std::byteswap(intervals) >> 16,
        .time2 = ((intervals >> 48) & 0x0FFF) | (0b0001'0000'0000'0000),
        .clockSeq = uuidClockSeq,
        .node = node,
    };

    return std::bit_cast<uuid>(result);
}

uint16_t uuid::v1ClockSeq() const noexcept {
    return (octets[8] & 0b0011'1111) << 8 | octets[9];
}

MacAddress uuid::v1Node() const noexcept {
    return uv1.node;
}

std::chrono::utc_clock::time_point uuid::v1Time() const noexcept {
    uint64_t intervals
        = uint64_t(uv1.time2 & 0x0FFF) << 48
        | uint64_t(uv1.time1.big()) << 32
        | uint64_t(uv1.time0.big());

    return chrono::clock_cast<chrono::utc_clock>(detail::rfc9562_clock::time_point{detail::rfc9562_clock::duration{intervals}});
}
