#pragma once

#include "logs/structured/category.hpp"

#include "logs/logs.hpp"

#include "fmt/compile.h"

#include <string_view>
#include <charconv>
#include <source_location>
#include <array>

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

        int indexAttributeCount;
        std::span<const MessageAttributeInfo> namedAttributes;

        size_t attributeCount() const noexcept { return indexAttributeCount + namedAttributes.size(); }
    };

    template<int N> requires (N >= 0)
    struct NumberToString {
        static constexpr size_t kBufferSize = (N / 10) + 1;
        static constexpr auto kText = []() {
            std::array<char, kBufferSize> result{};
            std::to_chars(result.data(), result.data() + result.size(), N);
            return result;
        }();

        static constexpr std::string_view value() noexcept {
            return std::string_view(kText.data(), kText.size());
        }
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

    struct InnerAttributeInfo {
        std::string_view name;
        int index;

        constexpr InnerAttributeInfo() noexcept
            : index(-1)
        { }

        constexpr InnerAttributeInfo(std::string_view name)
            : name(name)
            , index(-1)
        { }

        constexpr InnerAttributeInfo(int index)
            : index(index)
        { }

        constexpr bool isIndex() const noexcept { return index != -1; }
    };

    template<size_t N>
    struct AttributeArray {
        InnerAttributeInfo attributes[N] = {};

        constexpr InnerAttributeInfo *begin() noexcept { return attributes; }
        constexpr InnerAttributeInfo *end() noexcept { return attributes + N; }
        constexpr const InnerAttributeInfo *begin() const noexcept { return attributes; }
        constexpr const InnerAttributeInfo *end() const noexcept { return attributes + N; }
        constexpr size_t size() const noexcept { return N; }
    };

    template<size_t L, size_t R>
    constexpr auto operator+(const AttributeArray<L>& lhs, const AttributeArray<R>& rhs) noexcept {
        AttributeArray<L + R> result{};

        for (size_t i = 0; i < L; i++) {
            result.attributes[i] = lhs.attributes[i];
        }

        for (size_t i = 0; i < R; i++) {
            result.attributes[L + i] = rhs.attributes[i];
        }

        return result;
    }

    constexpr AttributeArray<0> makeFormatAttributesInner(fmt::detail::text<char>) noexcept {
        return {};
    }

    template<typename T, int N>
    constexpr AttributeArray<1> makeFormatAttributesInner(fmt::detail::field<char, T, N> field) noexcept {
        InnerAttributeInfo info{ N + 1 };
        return AttributeArray<1>{ info };
    }

    constexpr AttributeArray<1> makeFormatAttributesInner(fmt::detail::runtime_named_field<char> field) noexcept {
        InnerAttributeInfo info{ std::string_view{field.name} };
        return AttributeArray<1>{ info };
    }

    constexpr AttributeArray<0> makeFormatAttributesInner(fmt::detail::code_unit<char> field) noexcept {
        return {};
    }

    template<typename L, typename R>
    constexpr auto makeFormatAttributesInner(fmt::detail::concat<L, R> fmt) noexcept {
        return makeFormatAttributesInner(fmt.lhs) + makeFormatAttributesInner(fmt.rhs);
    }

    template<size_t N>
    struct PartialAttributeArray {
        MessageAttributeInfo attributes[N] = {};
        int indices;

        constexpr size_t size() const noexcept { return N + indices; }
        constexpr std::span<const MessageAttributeInfo> namedAttributes() const noexcept { return { attributes, N }; }
    };

    constexpr size_t largestIndexAttribute(std::span<const InnerAttributeInfo> attributes) noexcept {
        int largest = 0;
        for (const auto& attr : attributes)
            if (attr.isIndex())
                largest = (std::max)(largest, attr.index);

        return largest;
    }

    template<size_t N>
    constexpr auto makeSortedAttributes(AttributeArray<N> result) noexcept {
        std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.name < rhs.name;
        });
        return result;
    }

    constexpr size_t uniqueAttributeNameCount(std::span<const InnerAttributeInfo> attributes) noexcept {
        if (attributes.size() == 0)
            return 0;

        std::string_view last;
        size_t count = 0;
        for (const auto& attr : attributes) {
            if (attr.isIndex())
                continue;

            if (attr.name != last) {
                count++;
                last = attr.name;
            }
        }

        return count;
    }

    template<size_t N>
    constexpr auto makePartialAttributeArray(std::span<const InnerAttributeInfo> attributes) noexcept {
        PartialAttributeArray<N> result{};
        result.indices = largestIndexAttribute(attributes);

        size_t i = 0;
        std::string_view last;
        for (const auto& attr : attributes) {
            if (attr.isIndex())
                continue;

            if (attr.name != last) {
                result.attributes[i++] = { attr.name };
                last = attr.name;
            }
        }

        return result;
    }

    template<typename... A>
    struct FormatWrapper {
        constexpr auto operator()(auto fmt) noexcept {
            constexpr auto compiled = fmt::detail::compile<A...>(fmt);
            constexpr auto array = makeFormatAttributesInner(compiled);
            constexpr auto result = makeSortedAttributes(array);
            constexpr size_t kCount = uniqueAttributeNameCount(std::span(result.begin(), result.end()));
            constexpr auto partial = makePartialAttributeArray<kCount>(std::span(result.begin(), result.end()));

            return partial;
        }
    };

    template<typename... A>
    constexpr FormatWrapper<A...> compileTupleArgs(A... args);
}

#define BUILD_MESSAGE_ATTRIBUTES_IMPL(message, ...) \
    []() constexpr { \
        using Wrapper = decltype(sm::logs::structured::compileTupleArgs(__VA_ARGS__)); \
        return (Wrapper{})(FMT_COMPILE(message)); \
    }()

#define PARSE_MESSAGE_ATTRIBUTES(message) \
    []() constexpr { \
        std::array<sm::logs::structured::MessageAttributeInfo, sm::logs::structured::detail::getAttributes(message).size()> result{}; \
        size_t i = 0; \
        for (const auto& attr : sm::logs::structured::detail::getAttributes(message)) { \
            result[i++] = attr; \
        } \
        return result; \
    }()
