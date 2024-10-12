#pragma once

#include <simcoe_config.h>

#include "logs.meta.hpp"

namespace sm::logs {
    REFLECT_ENUM(Severity)
    enum class Severity {
        eTrace = 0,
        eDebug = 1,
        eInfo = 2,
        eWarning = 3,
        eError = 4,
        eFatal = 5,
        ePanic = 6,

        eCount
    };
} // namespace sm::logs
