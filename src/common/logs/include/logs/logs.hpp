#pragma once

#include <simcoe_config.h>

#include "logs.reflect.h"

namespace sm::logs {
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

} // namespace sm::logs

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

#   if SMC_ENABLE_LOGDB
template <size_t N>
auto message(const char (&str)[N]) -> logdb::LogString<N> {
    constexpr auto msg = logdb::build_log_message(str);
    logdb::add_message(str, msg.hash);
    return msg;
}
#   elif SMC_DEBUG
template <size_t N>
consteval auto message(const char (&str)[N]) -> logdb::LogString<N> {
    ctu::SmallString<N> msg{str};
    return {msg, 0};
}
#   elif SMC_RELEASE
template <size_t N>
consteval auto message(const char (&str)[N]) -> logdb::LogString<N> {
    constexpr auto msg = logdb::build_log_message(str);
    logdb::LogString<N> result;
    result.append_int(msg.hash, 16);
    result.append(msg.args.data());
    return result;
}
#   endif

#   if SMC_ENABLE_LOGDB
#      define SM_MESSAGE(id, text) static const auto id = sm::message(text)
#   else
#      define SM_MESSAGE(id, text) static constexpr auto id = sm::message(text)
#   endif

#endif
