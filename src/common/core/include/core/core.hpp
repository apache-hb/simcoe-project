#pragma once

#include <stdint.h>

namespace sm {
    using uint8 = uint8_t; // NOLINT
    using uint16 = uint16_t; // NOLINT
    using uint32 = uint32_t; // NOLINT
    using uint64 = uint64_t; // NOLINT

    using int8 = int8_t; // NOLINT
    using int16 = int16_t; // NOLINT
    using int32 = int32_t; // NOLINT
    using int64 = int64_t; // NOLINT

    using uint = uint32_t; // NOLINT

    template<typename T>
    concept StandardLayout = __is_standard_layout(T);
}
