#pragma once

#include <simcoe_config.h>

#include <string_view>
#include <source_location>
#include <variant>

#include "logs/logs.hpp"
#include "orm/error.hpp"

#include <fmtlib/format.h>
#include <fmt/compile.h>

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
        uint32_t line;
        std::string_view file;
        std::string_view function;
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
            std::vector<MessageAttributeInfo> attributes;
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
        void fmtMessage(fmt::format_string<A...> fmt, A&&... args) noexcept {
            const auto& message = gTagInfo<F, A...>;
            postLogMessage(message, fmt::make_format_args(args...));
        }
    }
} // namespace sm::logs

#define LOG_MESSAGE(severity, message, ...) \
    do { \
        static constexpr std::string_view fnName = __FUNCTION__; \
        struct LogMessageImpl { \
            constexpr sm::logs::structured::LogMessageInfo operator()() const noexcept { \
                return { UINT64_MAX, severity, message, __LINE__, __FILE__, fnName }; \
            } \
        }; \
        sm::logs::structured::detail::fmtMessage<LogMessageImpl>(message __VA_OPT__(,) __VA_ARGS__); \
    } while (false)

#define LOG_TRACE(...) LOG_MESSAGE(sm::logs::Severity::eTrace, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_MESSAGE(sm::logs::Severity::eDebug, __VA_ARGS__)
#define LOG_INFO(...) LOG_MESSAGE(sm::logs::Severity::eInfo, __VA_ARGS__)
#define LOG_WARN(...) LOG_MESSAGE(sm::logs::Severity::eWarning, __VA_ARGS__)
#define LOG_ERROR(...) LOG_MESSAGE(sm::logs::Severity::eError, __VA_ARGS)
#define LOG_FATAL(...) LOG_MESSAGE(sm::logs::Severity::eFatal, __VA_ARGS)
#define LOG_PANIC(...) LOG_MESSAGE(sm::logs::Severity::ePanic, __VA_ARGS)
