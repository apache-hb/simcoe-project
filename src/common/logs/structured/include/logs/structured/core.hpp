#pragma once

#include "logs/structured/category.hpp"

#include "logs/logs.hpp"

#include <string_view>
#include <source_location>

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

    namespace detail {
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
    }
}

#define PARSE_MESSAGE_ATTRIBUTES(message) \
    []() constexpr { \
        std::array<sm::logs::structured::MessageAttributeInfo, sm::logs::structured::detail::getAttributes(message).size()> result{}; \
        size_t i = 0; \
        for (const auto& attr : sm::logs::structured::detail::getAttributes(message)) { \
            result[i++] = attr; \
        } \
        return result; \
    }()
