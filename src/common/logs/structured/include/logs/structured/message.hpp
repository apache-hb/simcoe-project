#pragma once

#include "logs/structured/category.hpp"
#include "logs/structured/params.hpp"

#include "logs/logs.hpp"

#include <string_view>
#include <source_location>

namespace sm::logs::structured {
    struct MessageInfo {
        uint64_t hash;
        logs::Severity level;
        const CategoryInfo& category;
        std::string_view message;
        std::source_location location;

        int indexAttributeCount;
        std::span<const MessageAttributeInfo> namedAttributes;

        consteval MessageInfo(
            std::string_view message, logs::Severity level,
            const CategoryInfo& category, std::source_location location,
            int indexAttributeCount, std::span<const MessageAttributeInfo> namedAttributes
        ) noexcept
            : hash(detail::hashMessage(message))
            , level(level)
            , category(category)
            , message(message)
            , location(location)
            , indexAttributeCount(indexAttributeCount)
            , namedAttributes(namedAttributes)
        { }

        size_t attributeCount() const noexcept { return indexAttributeCount + namedAttributes.size(); }
    };

    namespace detail {
        struct MessageId {
            MessageId(const MessageInfo& message) noexcept;
            const MessageInfo& info;
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
