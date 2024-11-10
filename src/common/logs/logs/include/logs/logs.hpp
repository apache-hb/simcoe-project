#pragma once

#include <simcoe_config.h>

#include "logs.meta.hpp"

namespace sm::logs {
    REFLECT_ENUM(Severity)
    enum class Severity {
        eTrace,
        eDebug,
        eInfo,
        eWarning,
        eError,
        eFatal,
        ePanic,

        eCount
    };
} // namespace sm::logs
