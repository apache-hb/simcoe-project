#pragma once

#include <simcoe_config.h>

#include "core/format.hpp"

#include "logs.reflect.h"

#include <iterator>

namespace sm {
class IArena;
}

namespace sm {
namespace logs {
class ILogger {
    Severity m_severity;

protected:
    constexpr ILogger(Severity severity)
        : m_severity(severity) {}

    virtual void accept(const Message &message) = 0;

public:
    virtual ~ILogger() = default;

    void log(Category category, Severity severity, std::string_view msg);
    void log(const Message &message);

    constexpr Severity get_severity() const {
        return m_severity;
    }
    constexpr bool will_accept(Severity severity) const {
        return severity >= m_severity;
    }
};

/// @brief a logging sink that wraps a logger and the category of the calling code
/// @tparam C the category of the calling code
/// @warning this class is not thread-safe
template <Category::Inner C>
    requires(Category{C}.is_valid())
class Sink final {
    mutable FormatPoolBuffer<Pool::eLogging> m_buffer;

    ILogger &m_logger;

public:
    constexpr Sink(ILogger &logger)
        : m_logger(logger) {}

    constexpr Sink(const Sink &other)
        : m_logger(other.m_logger) {}

    void operator()(Severity severity, std::string_view msg, auto &&...args) const {
        log(severity, msg, std::forward<decltype(args)>(args)...);
    }

    void log(Severity severity, std::string_view msg, auto &&...args) const {
        // while ILogger will reject the message, still do an early return
        // to avoid formatting if we don't need to
        if (m_logger.will_accept(severity)) {
            auto out = std::back_inserter(m_buffer);
            fmt::vformat_to(out, msg, fmt::make_format_args(args...));

            m_logger.log(C, severity, std::string_view{m_buffer.data(), m_buffer.size()});

            m_buffer.clear();
        }
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
} // namespace sm

#if 0
namespace logdb {
void add_message(const char *str, uint32_t hash);

// max length of a uint32_t converted to a string in base 16
static constexpr size_t kMinLogSize = 8;

template <size_t N>
struct LogString {
    ctu::SmallString<N> args;
    uint32_t hash;

    consteval operator std::string_view() const {
        return {args.data(), args.size()};
    }
};

template <size_t N>
consteval auto build_log_message(const char (&str)[N]) -> LogString<N> {
    ctu::SmallString<N> args{};
    uint32_t hash = 0;

    enum {
        eText,
        eStartEscape,
        eEscape,
    } state = eText;

    for (size_t i = 0; i < N; i++) {
        char c = str[i];
        hash = hash * 31 + c;
        if (c == '{') {
            switch (state) {
            case eText: state = eStartEscape; break;
            case eStartEscape: state = eText; break;
            default: NEVER("%s", "invalid escape sequence"); break;
            }
        } else if (c == '}') {
            switch (state) {
            case eText: break;
            default:
                args.append('}');
                state = eText;
                break;
            }
        } else {
            switch (state) {
            case eText: break;
            case eStartEscape:
                args.append('{');
                args.append(c);
                state = eEscape;
                break;
            case eEscape: args.append(c); break;
            }
        }
    }

    return LogString<N>{args, hash};
}
} // namespace logdb

#if SMC_ENABLE_LOGDB
template <size_t N>
auto message(const char (&str)[N]) -> logdb::LogString<N> {
    constexpr auto msg = logdb::build_log_message(str);
    logdb::add_message(str, msg.hash);
    return msg;
}
#elif SMC_DEBUG
template <size_t N>
consteval auto message(const char (&str)[N]) -> logdb::LogString<N> {
    ctu::SmallString<N> msg{str};
    return {msg, 0};
}
#elif SMC_RELEASE
template <size_t N>
consteval auto message(const char (&str)[N]) -> logdb::LogString<N> {
    constexpr auto msg = logdb::build_log_message(str);
    logdb::LogString<N> result;
    result.append_int(msg.hash, 16);
    result.append(msg.args.data());
    return result;
}
#endif

#if SMC_ENABLE_LOGDB
#   define SM_MESSAGE(id, text) static const auto id = sm::message(text)
#else
#   define SM_MESSAGE(id, text) static constexpr auto id = sm::message(text)
#endif

#endif
