#include "core/uuid.hpp"

#include <fmtlib/format.h>
#include <fmt/chrono.h>

using namespace sm;

namespace chrono = std::chrono;

using namespace std::chrono_literals;

// time handling
static constexpr std::chrono::system_clock::duration distance(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to) {
    auto begin = from.time_since_epoch();
    auto end = to.time_since_epoch();

    return (begin > end) ? begin - end : end - begin;
}


detail::rfc9562_clock::time_point detail::rfc9562_clock::from_sys(std::chrono::system_clock::time_point time) {
    // our epoch is 1582-10-15T00:00:00Z, the gregorian reform
    // we need to convert from the system clock epoch to the rfc9562 epoch.

    CTASSERTF(time >= kGregorianReform, "%s is before the gregorian reform, 1582-10-15", fmt::format("{}", time).c_str());

    return rfc9562_clock::time_point{distance(kGregorianReform, time)};
}

std::chrono::system_clock::time_point detail::rfc9562_clock::to_sys(time_point time) {
    return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>((kGregorianReform.time_since_epoch() + time.time_since_epoch()))};
}

// v1

static constexpr uint16_t kClockSeqMask = 0b0011'1111'1111'1111;

uuid uuid::v1(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node) {
    detail::rfc9562_clock::time_point clockOffset = chrono::clock_cast<detail::rfc9562_clock>(time);

    uint64_t intervals = clockOffset.time_since_epoch().count();

    uint16_t uuidClockSeq
        = (uuid::eDCE << 14) // 64:65
        | (clockSeq & kClockSeqMask); // 66:79

    uuidv1 result = {
        .time0 = intervals,
        .time1 = intervals >> 32,
        .time2 = ((intervals >> 48) & 0x0FFF) | (uuid::eVersion1 << 12),
        .clockSeq = uuidClockSeq,
        .node = node,
    };

    return std::bit_cast<uuid>(result);
}

uint16_t uuid::v1ClockSeq() const noexcept {
    return uv1.clockSeq & kClockSeqMask;
}

MacAddress uuid::v1Node() const noexcept {
    return uv1.node;
}

std::chrono::utc_clock::time_point uuid::v1Time() const noexcept {
    uint64_t intervals
        = uint64_t(uv1.time2 & 0x0FFF) << 48
        | uint64_t(uv1.time1) << 32
        | uint64_t(uv1.time0);

    return chrono::clock_cast<chrono::utc_clock>(detail::rfc9562_clock::time_point{detail::rfc9562_clock::duration{intervals}});
}

// v6

uuid uuid::v6(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node) {
    detail::rfc9562_clock::time_point clockOffset = chrono::clock_cast<detail::rfc9562_clock>(time);

    uint64_t intervals = clockOffset.time_since_epoch().count();

    uint16_t uuidClockSeq
        = (uuid::eDCE << 14) // 64:65
        | (clockSeq & kClockSeqMask); // 66:79

    uuidv6 result = {
        .time2 = intervals >> 28,
        .time1 = intervals >> 12,
        .time0 = (intervals & 0x0FFF) | (uuid::eVersion6 << 12),
        .clockSeq = uuidClockSeq,
        .node = node,
    };

    return std::bit_cast<uuid>(result);
}

uint16_t uuid::v6ClockSeq() const noexcept {
    return uv6.clockSeq & kClockSeqMask;
}

MacAddress uuid::v6Node() const noexcept {
    return uv6.node;
}

std::chrono::utc_clock::time_point uuid::v6Time() const noexcept {
    uint64_t intervals
        = uint64_t(uv6.time2) << 28
        | uint64_t(uv6.time1) << 12
        | uint64_t(uv6.time0 & 0x0FFF);

    return chrono::clock_cast<chrono::utc_clock>(detail::rfc9562_clock::time_point{detail::rfc9562_clock::duration{intervals}});
}

// v7

uuid uuid::v7(std::chrono::system_clock::time_point time, const uint8_t random[10]) {
    long long ts = chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch()).count();
    CTASSERTF(ts > 0, "%s is before the unix epoch, 1970-01-01.", fmt::format("{}", time).c_str());

    // TODO: this may be a touch innacurate
    uuidv7 result = {
        .time0 = ts >> 16,
        .time1 = ts,
    };

    memcpy(result.rand, random, sizeof(result.rand));
    result.rand[0] = (result.rand[0] & 0b0000'1111) | uuid::eVersion7 << 4;
    result.rand[2] = (result.rand[2] & 0b0011'1111) | uuid::eDCE << 6;

    return std::bit_cast<uuid>(result);
}

std::chrono::system_clock::time_point uuid::v7Time() const noexcept {
    uint64_t ts
        = uint64_t(uv7.time0) << 16
        | uint64_t(uv7.time1);

    return chrono::system_clock::time_point{chrono::milliseconds{ts}};
}
