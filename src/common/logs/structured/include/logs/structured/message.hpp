#pragma once

#include <simcoe_config.h>

#include <string_view>
#include <array>
#include <source_location>

#include "logs/logs.hpp"
#include "db/error.hpp"
#include "core/adt/small_vector.hpp"

#include "logs/structured/category.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    static constexpr size_t kMaxMessageAttributes = 8;

    struct MessageAttributeInfo {
        std::string_view name;
    };

    struct LogMessageInfo {
        uint64_t hash;
        logs::Severity level;
        const Category& category;
        std::string_view message;
        std::source_location location;
        std::span<const MessageAttributeInfo> attributes;
    };

    db::DbError setup(db::Connection& connection);
    bool isRunning() noexcept;
    void cleanup();

    namespace detail {
        template<typename T>
        concept LogMessageFn = requires(T fn) {
            { fn() } -> std::same_as<LogMessageInfo>;
        };

        constexpr static std::string_view kIndices[structured::kMaxMessageAttributes] = {
            "0", "1", "2", "3", "4", "5", "6", "7"
        };

        consteval std::vector<MessageAttributeInfo> getAttributes(std::string_view message) noexcept {
            std::vector<MessageAttributeInfo> attributes;

            size_t i = 0;
            while (i < message.size()) {
                auto start = message.find_first_of('{', i);
                if (start == std::string_view::npos) {
                    break;
                }

                auto end = message.find_first_of('}', start);
                if (end == std::string_view::npos) {
                    break;
                }

                auto id = message.substr(start + 1, end - start - 1);
                if (id.empty())
                    id = kIndices[attributes.size()];

                MessageAttributeInfo info {
                    .name = id
                };

                attributes.emplace_back(info);
                i = end + 1;
            }

            return attributes;
        }

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

#define PARSE_MESSAGE_ATTRIBUTES(message) \
    []() constexpr { \
        std::array<sm::logs::structured::MessageAttributeInfo, sm::logs::structured::detail::getAttributes(message).size()> result{}; \
        size_t i = 0; \
        for (const auto& attr : sm::logs::structured::detail::getAttributes(message)) { \
            result[i++] = attr; \
        } \
        return result; \
    }()

#define LOG_MESSAGE_CATEGORY(id, name) \
    struct id final : public sm::logs::Category { \
        constexpr id() noexcept \
            : Category(sm::logs::detail::LogCategoryData{name, __LINE__}) \
        { } \
    }

#define LOG_MESSAGE(category, severity, message, ...) \
    do { \
        static constexpr std::source_location loc = std::source_location::current(); \
        static constexpr auto attrs = PARSE_MESSAGE_ATTRIBUTES(message); \
        struct LogMessageImpl { \
            constexpr sm::logs::structured::LogMessageInfo operator()() const noexcept { \
                return { sm::logs::detail::hashMessage(message, loc.line()), severity, sm::logs::structured::detail::gLogCategory<category>, message, loc, attrs }; \
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
