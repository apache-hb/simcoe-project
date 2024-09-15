#pragma once

#include "structured_log_config.h"

#include <string_view>
#include <source_location>
#include <array>

#include "logs/logs.hpp"
#include "db/error.hpp"
#include "core/adt/small_vector.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    static constexpr size_t kMaxMessageAttributes = 8;

    struct MessageAttributeInfo {
#if SMC_LOG_CONTENT
        std::string_view name;
#endif
    };

    using AttributeInfoVec = sm::SmallVector<MessageAttributeInfo, structured::kMaxMessageAttributes>;

    struct LogMessageInfo {
        uint64_t hash;

#if SMC_LOG_CONTENT
        logs::Severity level;
        std::string_view message;
        std::source_location location;
        std::span<const MessageAttributeInfo> attributes;
#else
        size_t attributeCount;
#endif

        constexpr size_t getAttributeCount() const noexcept {
#if SMC_LOG_CONTENT
            return attributes.size();
#else
            return attributeCount;
#endif
        }
    };

    db::DbError setup(db::Connection& connection);
    void cleanup();

    namespace detail {
        consteval uint64_t hashMessage(std::string_view message) noexcept {
            uint64_t hash = 0;
            for (char c : message) {
                hash = (hash * 31) + c;
            }
            return hash;
        }

        consteval size_t countAttributes(const char *text) noexcept {
            size_t count = 0;
            size_t i = 0;
            std::string_view message{text};
            while (i < message.size()) {
                auto start = message.find_first_of('{', i);
                if (start == std::string_view::npos) {
                    break;
                }

                auto end = message.find_first_of('}', start);
                if (end == std::string_view::npos) {
                    break;
                }

                count += 1;
                i = end + 1;
            }
            return count;
        }

        template<size_t N>
        consteval std::array<MessageAttributeInfo, N> getAttributes(std::string_view message) noexcept {
            std::array<MessageAttributeInfo, N> attributes{};
            size_t index = 0;

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

                MessageAttributeInfo info {
#if SMC_LOG_CONTENT
                    .name = message.substr(start + 1, end - start - 1)
#endif
                };

                attributes[index++] = info;
                i = end + 1;
            }

            return attributes;
        }

        template<typename T>
        concept LogMessageFn = requires(T fn) {
            { fn() } -> std::same_as<LogMessageInfo>;
        };

        struct LogMessageId {
            LogMessageId(const LogMessageInfo& message) noexcept;
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

        void postLogMessage(const LogMessageId& message, sm::SmallVectorBase<std::string> args) noexcept;

        template<LogMessageFn F, typename... A>
        void fmtMessage(A&&... args) noexcept {
            constexpr size_t argCount = sizeof...(A);
            static_assert(argCount <= kMaxMessageAttributes, "Too many message attributes");

            const LogMessageId& message = gTagInfo<F, A...>;
            [[maybe_unused]] size_t attribCount = message.info.getAttributeCount();
            CTASSERTF(argCount == attribCount, "Incorrect number of message attributes %zu != %zu", argCount, attribCount);

            sm::SmallVector<std::string, argCount> argArray{};
            ((argArray.emplace_back(fmt::to_string(std::forward<A>(args)))), ...);

            postLogMessage(message, argArray);
        }
    }
} // namespace sm::logs

#if SMC_LOG_CONTENT
#   define LOG_MESSAGE_BODY(SEVERITY, MESSAGE) \
    static constexpr std::string_view message = (MESSAGE); \
    static constexpr auto hash = sm::logs::structured::detail::hashMessage(message); \
    static constexpr auto attrs = sm::logs::structured::detail::getAttributes<sm::logs::structured::detail::countAttributes(MESSAGE)>(message); \
    return sm::logs::structured::LogMessageInfo { .hash = hash, .level = (SEVERITY), .message = message, .location = loc, .attributes = attrs, };
#else
#   define LOG_MESSAGE_BODY(SEVERITY, MESSAGE) \
    static constexpr std::string_view message = (MESSAGE); \
    static constexpr auto hash = sm::logs::structured::detail::hashMessage(message); \
    static constexpr auto count = sm::logs::structured::detail::countAttributes(MESSAGE); \
    return sm::logs::structured::LogMessageInfo { .hash = hash, .attributeCount = count, };
#endif

#define LOG_MESSAGE(SEVERITY, MESSAGE, ...) \
    do { \
        [[maybe_unused]] static constexpr std::source_location loc = std::source_location::current(); \
        struct LogMessageImpl { \
            constexpr sm::logs::structured::LogMessageInfo operator()() const noexcept { \
                LOG_MESSAGE_BODY(SEVERITY, MESSAGE) \
            } \
        }; \
        sm::logs::structured::detail::fmtMessage<LogMessageImpl>(__VA_ARGS__); \
    } while (false)

#define LOG_TRACE(...) LOG_MESSAGE(sm::logs::Severity::eTrace,  __VA_ARGS__)
#define LOG_DEBUG(...) LOG_MESSAGE(sm::logs::Severity::eDebug,  __VA_ARGS__)
#define LOG_INFO(...) LOG_MESSAGE(sm::logs::Severity::eInfo,    __VA_ARGS__)
#define LOG_WARN(...) LOG_MESSAGE(sm::logs::Severity::eWarning, __VA_ARGS__)
#define LOG_ERROR(...) LOG_MESSAGE(sm::logs::Severity::eError,  __VA_ARGS__)
#define LOG_FATAL(...) LOG_MESSAGE(sm::logs::Severity::eFatal,  __VA_ARGS__)
#define LOG_PANIC(...) LOG_MESSAGE(sm::logs::Severity::ePanic,  __VA_ARGS__)
