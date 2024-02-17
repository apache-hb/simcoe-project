#pragma once

#include "logs/logs.hpp"

#include "fmtlib/format.h"

namespace sm::logs {

/// @brief a logging sink that wraps a logger and the category of the calling code
/// @tparam C the category of the calling code
/// @warning this class is not thread-safe
template <Category::Inner C> requires(Category{C}.is_valid())
class Sink final {
    ILogger &m_logger;

public:
    constexpr Sink(ILogger &logger)
        : m_logger(logger)
    { }

    constexpr Sink(const Sink &other)
        : m_logger(other.m_logger)
    { }

    void log(Severity severity, std::string_view msg, auto &&...args) const {
        // while ILogger will reject the message, still do an early return
        // to avoid formatting if we don't need to
        if (m_logger.will_accept(severity)) {
            auto text = fmt::vformat(msg, fmt::make_format_args(args...));

            m_logger.log(C, severity, text);
        }
    }

    void operator()(Severity severity, std::string_view msg, auto &&...args) const {
        log(severity, msg, std::forward<decltype(args)>(args)...);
    }

    void trace(std::string_view msg, auto &&...args) const {
        log(Severity::eTrace, msg, std::forward<decltype(args)>(args)...);
    }

    void debug(std::string_view msg, auto &&...args) const {
        log(Severity::eInfo, msg, std::forward<decltype(args)>(args)...);
    }

    void info(std::string_view msg, auto &&...args) const {
        log(Severity::eInfo, msg, std::forward<decltype(args)>(args)...);
    }

    void warn(std::string_view msg, auto &&...args) const {
        log(Severity::eWarning, msg, std::forward<decltype(args)>(args)...);
    }

    void error(std::string_view msg, auto &&...args) const {
        log(Severity::eError, msg, std::forward<decltype(args)>(args)...);
    }

    void fatal(std::string_view msg, auto &&...args) const {
        log(Severity::eFatal, msg, std::forward<decltype(args)>(args)...);
    }

    void panic(std::string_view msg, auto &&...args) const {
        log(Severity::ePanic, msg, std::forward<decltype(args)>(args)...);
    }
};
} // namespace logs
