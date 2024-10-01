#pragma once

#include <simcoe_config.h>

#include <string_view>
#include <array>

#include "fmt/args.h"

#include "logs/logs.hpp"
#include "db/error.hpp"

#include "logs/structured/category.hpp"
#include "logs/structured/core.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    db::DbError setup(db::Connection& connection);
    bool isRunning() noexcept;
    void cleanup();

    namespace detail {
        using ArgStore = fmt::dynamic_format_arg_store<fmt::format_context>;

        template<typename T>
        concept LogMessageFn = requires(T fn) {
            { fn() } -> std::same_as<LogMessageInfo>;
        };

        struct LogMessageId {
            LogMessageId(LogMessageInfo& message) noexcept;
            const LogMessageInfo& info;
        };

        template<LogMessageFn F, typename... A>
        LogMessageInfo& getMessage() noexcept {
            F fn{};
            static LogMessageInfo message{fn()};
            return message;
        }

        template<LogMessageFn F, typename... A>
        inline LogMessageId gTagInfo{getMessage<F, A...>()};

        template<typename T>
        inline Category gLogCategory = T{};

        void postLogMessage(const LogMessageInfo& message, ArgStore args) noexcept;

        template<LogMessageFn F, typename... A>
        void fmtMessage(A&&... args) noexcept {
            const LogMessageId& message = gTagInfo<F, A...>;
            const LogMessageInfo& info = message.info;

            ArgStore store;
            store.reserve(info.attributeCount(), info.namedAttributes.size());
            ((store.push_back(std::forward<A>(args))), ...);

            postLogMessage(info, std::move(store));
        }
    }
} // namespace sm::logs

#define LOG_MESSAGE_CATEGORY(id, name) \
    struct id final : public sm::logs::Category { \
        constexpr id() noexcept \
            : Category(sm::logs::detail::LogCategoryData{name, __LINE__}) \
        { } \
    }

#define LOG_MESSAGE(category, severity, message, ...) \
    do { \
        static constexpr std::source_location loc = std::source_location::current(); \
        static constexpr auto attrs = BUILD_MESSAGE_ATTRIBUTES_IMPL(message, __VA_ARGS__); \
        struct LogMessageImpl { \
            constexpr sm::logs::structured::LogMessageInfo operator()() const noexcept { \
                return { sm::logs::detail::hashMessage(message, loc.line()), severity, sm::logs::structured::detail::gLogCategory<category>, message, loc, attrs.indices, attrs.namedAttributes() }; \
            } \
        }; \
        sm::logs::structured::detail::fmtMessage<LogMessageImpl>(__VA_ARGS__); \
    } while (false)

#define LOG_TRACE(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eTrace,   __VA_ARGS__)
#define LOG_DEBUG(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eDebug,   __VA_ARGS__)
#define LOG_INFO(category, ...)  LOG_MESSAGE(category, sm::logs::Severity::eInfo,    __VA_ARGS__)
#define LOG_WARN(category, ...)  LOG_MESSAGE(category, sm::logs::Severity::eWarning, __VA_ARGS__)
#define LOG_ERROR(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eError,   __VA_ARGS__)
#define LOG_FATAL(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eFatal,   __VA_ARGS__)
#define LOG_PANIC(category, ...) LOG_MESSAGE(category, sm::logs::Severity::ePanic,   __VA_ARGS__)

LOG_MESSAGE_CATEGORY(GlobalLog, "Global");
