#pragma once

#include <simcoe_logs_config.h>

#include "logs/category.hpp"
#include "logs/params.hpp"

#include "logs/logs.hpp"

#include <string_view>
#include <source_location>

#if SMC_LOGS_INCLUDE_SOURCE_INFO
#   define SM_CURRENT_SOURCE_LOCATION() std::source_location::current()
#else
#   define SM_CURRENT_SOURCE_LOCATION() std::source_location{}
#endif

namespace sm::logs {
    class MessageInfo {
        uint64_t hash;
        logs::Severity level;
        const CategoryInfo& category;
        std::string_view message;

#if SMC_LOGS_INCLUDE_SOURCE_INFO
        std::source_location location;
#endif

    public:

        int indexAttributeCount;
        std::span<const MessageAttributeInfo> namedAttributes;

        consteval MessageInfo(
            std::string_view message,
            logs::Severity level,
            const CategoryInfo& category,
            [[maybe_unused]] std::source_location location,
            int indexAttributeCount,
            std::span<const MessageAttributeInfo> namedAttributes
        ) noexcept
            : hash(detail::hashMessage(message))
            , level(level)
            , category(category)
            , message(message)
#if SMC_LOGS_INCLUDE_SOURCE_INFO
            , location(location)
#endif
            , indexAttributeCount(indexAttributeCount)
            , namedAttributes(namedAttributes)
        { }

        logs::Severity getSeverity() const noexcept { return level; }
        uint64_t getHash() const noexcept { return hash; }
        const CategoryInfo& getCategory() const noexcept { return category; }
        std::string_view getMessage() const noexcept { return message; }

#if SMC_LOGS_INCLUDE_SOURCE_INFO
        uint_least32_t getLine() const noexcept { return location.line(); }
        const char *getFunction() const noexcept { return location.function_name(); }
        const char *getFileName() const noexcept { return location.file_name(); }
#else
        uint_least32_t getLine() const noexcept { return 0; }
        const char *getFunction() const noexcept { return ""; }
        const char *getFileName() const noexcept { return ""; }
#endif
        size_t attributeCount() const noexcept { return indexAttributeCount + namedAttributes.size(); }
    };

    namespace detail {
        struct MessageId {
            MessageId(const MessageInfo& info) noexcept;
            const MessageInfo& data;
        };

        template<typename T>
        concept LogMessageFn = requires(T fn) {
            { fn() } -> std::same_as<MessageInfo>;
        };

        template<LogMessageFn F, typename... A>
        MessageInfo& getMessage() noexcept {
            F fn{};
            static MessageInfo message{fn()};
            return message;
        }

        template<LogMessageFn F, typename... A>
        inline MessageId gTagInfo{getMessage<F, A...>()};
    }
}