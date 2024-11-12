#pragma once

#include <simcoe_config.h>
#include <simcoe_logs_config.h>

#include <string_view>

#include <fmtlib/format.h>

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

template<>
struct fmt::formatter<sm::logs::Severity> {
    constexpr auto parse(format_parse_context& ctx) const {
        return ctx.begin();
    }

    constexpr auto format(const sm::logs::Severity& severity, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "{}", sm::logs::toString(severity));
    }
};
