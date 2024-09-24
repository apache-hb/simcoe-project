#pragma once

#include <simcoe_config.h>

#include <string_view>
#include <source_location>

#include "logs/logs.hpp"
#include "db/error.hpp"
#include "core/adt/small_vector.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    static constexpr size_t kMaxMessageAttributes = 8;

    namespace detail {
        consteval uint64_t hashMessage(std::string_view message, uint64_t seed) noexcept {
            uint64_t hash = seed;
            for (char c : message) {
                hash = (hash * 31) + c;
            }
            return hash;
        }
    }

    struct LogCategoryData {
        std::string_view name;
        uint64_t hash;

        consteval LogCategoryData(std::string_view name, uint64_t line) noexcept
            : name(name)
            , hash(detail::hashMessage(name, line))
        { }
    };

    struct LogCategory {
        const LogCategoryData data;

        LogCategory(LogCategoryData data) noexcept;

        constexpr uint64_t hash() const noexcept { return data.hash; }
    };

    struct MessageAttributeInfo {
        std::string_view name;
    };

    struct LogMessageInfo {
        uint64_t hash;
        logs::Severity level;
        const LogCategory& category;
        std::string_view message;
        std::source_location location;
        sm::SmallVector<MessageAttributeInfo, kMaxMessageAttributes> attributes;
    };

    db::DbError setup(db::Connection& connection);
    bool isRunning() noexcept;
    void cleanup();

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

        template<typename T>
        inline LogCategory gLogCategory = T{};

        void postLogMessage(const LogMessageId& message, sm::SmallVectorBase<std::string> args) noexcept;

        template<LogMessageFn F, typename... A>
        void fmtMessage(A&&... args) noexcept {
            constexpr size_t argCount = sizeof...(A);
            static_assert(argCount <= kMaxMessageAttributes, "Too many message attributes");

            const LogMessageId& message = gTagInfo<F, A...>;
            size_t attribCount = message.info.attributes.size();
            CTASSERTF(argCount == attribCount, "Incorrect number of message attributes %zu != %zu", argCount, attribCount);

            sm::SmallVector<std::string, argCount> argArray{};
            ((argArray.emplace_back(fmt::to_string(std::forward<A>(args)))), ...);

            postLogMessage(message, argArray);
        }
    }
} // namespace sm::logs

#define LOG_MESSAGE_CATEGORY(id, name) \
    struct id final : public sm::logs::structured::LogCategory { \
        constexpr id() noexcept \
            : LogCategory(sm::logs::structured::LogCategoryData{name, __LINE__}) \
        { } \
    }

#define LOG_MESSAGE(category, severity, message, ...) \
    do { \
        static constexpr std::source_location loc = std::source_location::current(); \
        struct LogMessageImpl { \
            constexpr sm::logs::structured::LogMessageInfo operator()() const noexcept { \
                return { sm::logs::structured::detail::hashMessage(message, loc.line()), severity, sm::logs::structured::detail::gLogCategory<category>, message, loc }; \
            } \
        }; \
        sm::logs::structured::detail::fmtMessage<LogMessageImpl>(__VA_ARGS__); \
    } while (false)

#define LOG_TRACE(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eTrace,   __VA_ARGS__)
#define LOG_DEBUG(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eDebug,   __VA_ARGS__)
#define LOG_INFO(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eInfo,    __VA_ARGS__)
#define LOG_WARN(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eWarning, __VA_ARGS__)
#define LOG_ERROR(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eError,   __VA_ARGS__)
#define LOG_FATAL(category, ...) LOG_MESSAGE(category, sm::logs::Severity::eFatal,   __VA_ARGS__)
#define LOG_PANIC(category, ...) LOG_MESSAGE(category, sm::logs::Severity::ePanic,   __VA_ARGS__)

LOG_MESSAGE_CATEGORY(GlobalLog, "Global");
