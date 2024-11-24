#pragma once

#include <simcoe_config.h>
#include <simcoe_logs_config.h>

#include <string_view>

#include "core/format.hpp"

namespace sm::logs {
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

    std::string_view toString(Severity severity) noexcept;
} // namespace sm::logs
