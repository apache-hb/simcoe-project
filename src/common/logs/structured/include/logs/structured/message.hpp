#pragma once

#include <simcoe_config.h>

#include <string_view>
#include <source_location>
#include <variant>

#include "logs/logs.hpp"
#include "orm/error.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    struct MessageAttributeInfo {
        uint64_t id;
        std::string_view name;
    };

    using MessageAttribute = std::variant<
        std::string,
        std::int64_t,
        float,
        bool
    >;

    struct LogMessageInfo {
        uint64_t id;
        logs::Severity level;
        std::string_view message;
        std::source_location location;
        std::vector<MessageAttributeInfo> attributes;
    };

    db::DbError setup(db::Connection& connection);

    namespace detail {
        template<typename T>
        concept LogMessageFn = requires(T fn) {
            { fn() } -> std::same_as<LogMessageInfo>;
        };

        struct LogMessageId {
            LogMessageId(LogMessageInfo& message) noexcept;
            LogMessageInfo& info;
        };

        template<LogMessageFn F, typename... A>
        LogMessageInfo& getMessage() noexcept {
            F fn{};
            static LogMessageInfo message{fn()};
            return message;
        }

        template<LogMessageFn F, typename... A>
        inline LogMessageId gTagInfo{getMessage<F, A...>()};

        void postLogMessage(const LogMessageId& message, fmt::format_args args) noexcept;

        template<LogMessageFn F, typename... A>
        void fmtMessage(std::string_view fmt, A&&... args) noexcept {
            postLogMessage(gTagInfo<F, A...>, fmt::make_format_args(args...));
        }
    }
} // namespace sm::logs

#define LOG_MESSAGE(severity, message, ...) \
    do { \
        static constexpr std::source_location loc = std::source_location::current(); \
        struct LogMessageImpl { \
            constexpr sm::logs::structured::LogMessageInfo operator()() const noexcept { \
                return { UINT64_MAX, severity, message, loc }; \
            } \
        }; \
        sm::logs::structured::detail::fmtMessage<LogMessageImpl>(message __VA_OPT__(,) __VA_ARGS__); \
    } while (false)

#define LOG_TRACE(...) LOG_MESSAGE(sm::logs::Severity::eTrace,  __VA_ARGS__)
#define LOG_DEBUG(...) LOG_MESSAGE(sm::logs::Severity::eDebug,  __VA_ARGS__)
#define LOG_INFO(...) LOG_MESSAGE(sm::logs::Severity::eInfo,    __VA_ARGS__)
#define LOG_WARN(...) LOG_MESSAGE(sm::logs::Severity::eWarning, __VA_ARGS__)
#define LOG_ERROR(...) LOG_MESSAGE(sm::logs::Severity::eError,  __VA_ARGS__)
#define LOG_FATAL(...) LOG_MESSAGE(sm::logs::Severity::eFatal,  __VA_ARGS__)
#define LOG_PANIC(...) LOG_MESSAGE(sm::logs::Severity::ePanic,  __VA_ARGS__)
