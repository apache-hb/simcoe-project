#include "core/uuid.hpp"

#include <fmtlib/format.h>
#include <fmt/chrono.h>

using namespace sm;

namespace chrono = std::chrono;

using namespace std::chrono_literals;

uuid uuid::v1(std::chrono::utc_clock::time_point time, uint16_t clockSeq, uint64_t node) {
    static constexpr chrono::system_clock::time_point kGregorianReform = chrono::sys_days{1582y/8/15};

    chrono::system_clock::time_point systime = chrono::clock_cast<chrono::system_clock>(time);
    chrono::utc_clock::duration clockOffset = systime - kGregorianReform;

    CTASSERTF(clockOffset.count() >= 0, "v1 uuids cannot represent times before 1582-8-15T00:00:00Z");
    chrono::nanoseconds ns = chrono::duration_cast<chrono::nanoseconds>(clockOffset);
    uint64_t intervals = ns.count() / 100;

    uint64_t uuidTime = ((intervals & 0xFF'FF'FF'FF'FF'FF) << 16) | (uuid::eVersion1 << 12) | ((intervals >> 48) & 0xFFFF);
    uint16_t uuidClockSeq = (clockSeq & 0b0011'1111'1111'1111) | (uuid::eRfc9562 << 14);

    uuidv1 result = {
        .time = uuidTime,
        .clockSeq = uuidClockSeq,
        .node0 = uint16_t(node & 0xFFFF),
        .node1 = uint32_t((node >> 16) & 0xFFFFFFFF),
    };

    return std::bit_cast<uuid>(result);
}
