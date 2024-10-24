#pragma once

#include <span>
#include <string_view>
#include <charconv>
#include <array>

#include "fmtlib/format.h"
#include "fmt/base.h"
#include "fmt/compile.h"
#include "fmt/args.h"
#include "fmt/std.h"

namespace sm::logs {
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

    using ArgStore = fmt::dynamic_format_arg_store<fmt::format_context>;

    using FormatArg = fmt::basic_format_arg<fmt::format_context>;
    using NamedArg = std::pair<std::string_view, FormatArg>;

    class DynamicArgStore {
        virtual FormatArg getArg(int index) const noexcept = 0;
        virtual FormatArg getArg(std::string_view name) const noexcept = 0;
        virtual ArgStore buildArgStore() const = 0;
    public:
        virtual ~DynamicArgStore() = default;

        FormatArg get(int index) const noexcept {
            return getArg(index);
        }

        FormatArg get(std::string_view name) const noexcept {
            return getArg(name);
        }

        ArgStore asDynamicArgStore() const {
            return buildArgStore();
        }
    };

    template<typename T>
    consteval bool isNamedArg() noexcept {
        return fmt::detail::is_named_arg<T>::value;
    }

    template<typename... A>
    constexpr int kNamedArgCount = fmt::detail::count_named_args<A...>();

    namespace detail {
        // c++ has some strange type rules around lvalue references to arrays
        // std::is_array_v doesnt handle them in the way i want, so i do it
        // manually here. note the T(&)[] cases
        template<typename>
        constexpr bool IsArray = false;

        template<typename T, size_t N>
        constexpr bool IsArray<T[N]> = true;

        template<typename T, size_t N>
        constexpr bool IsArray<T(&)[N]> = true;

        template<typename T>
        constexpr bool IsArray<T[]> = true;

        template<typename T>
        constexpr bool IsArray<T(&)[]> = true;

        template<typename T>
        struct ArgData : std::remove_cvref<T> { };

        // while this does not cover all array cases, i only
        // use char[] as arrays, so string covers all usecases.
        // more specializations can be added as needed.
        template<typename T> requires (IsArray<T>)
        struct ArgData<T> {
            using type = std::string;
        };

        template<typename T>
        using ArgDataT = typename ArgData<T>::type;
    }

    template<typename... A>
    using ArgTuple = std::tuple<detail::ArgDataT<A>...>;

    template<typename... A>
    class ArgStoreData final : public DynamicArgStore {
        using InnerData = ArgTuple<A...>;
        InnerData args;

        template<size_t N>
        using ArgType = std::tuple_element_t<N, InnerData>;

        template<size_t N = 0>
        FormatArg getArgByIndex(int index) const noexcept {
            if (index == N)
                return FormatArg(std::get<N>(args));

            return getArgByIndex<N + 1>(index);
        }

        template<>
        FormatArg getArgByIndex<sizeof...(A)>(int) const noexcept {
            return {};
        }

        FormatArg getArg(int index) const noexcept override {
            return getArgByIndex(index);
        }

        template<size_t N = 0>
        FormatArg getArgByName(std::string_view name) const noexcept {
            if constexpr (isNamedArg<ArgType<N>>()) {
                if (std::get<N>(args).name == name)
                    return FormatArg(std::get<N>(args).value);
            }

            return getArgByName<N + 1>(name);
        }

        template<>
        FormatArg getArgByName<sizeof...(A)>(std::string_view) const noexcept {
            return {};
        }

        FormatArg getArg(std::string_view name) const noexcept override {
            return getArgByName(name);
        }

        ArgStore buildArgStore() const override {
            constexpr int kNamedCount = kNamedArgCount<A...>;

            ArgStore store;
            store.reserve(sizeof...(A) - kNamedCount, kNamedCount);
            std::apply([&store](const auto&... args) {
                (store.push_back(args), ...);
            }, args);
            return store;
        }

    public:
        ArgStoreData(A&&... args) noexcept
            : args(args...)
        { }
    };

    struct MessageAttributeInfo {
        std::string_view name;
    };

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

    constexpr AttributeArray<0> makeFormatAttributesInner(fmt::detail::code_unit<char> field) noexcept {
        return {};
    }

    template<typename T, int N>
    constexpr AttributeArray<1> makeFormatAttributesInner(fmt::detail::field<char, T, N> field) noexcept {
        InnerAttributeInfo info{ N + 1 };
        return AttributeArray<1>{ info };
    }

    template<typename T, int N>
    constexpr AttributeArray<1> makeFormatAttributesInner(fmt::detail::spec_field<char, T, N> field) noexcept {
        InnerAttributeInfo info{ N + 1 };
        return AttributeArray<1>{ info };
    }

    constexpr AttributeArray<1> makeFormatAttributesInner(fmt::detail::runtime_named_field<char> field) noexcept {
        InnerAttributeInfo info{ std::string_view{field.name} };
        return AttributeArray<1>{ info };
    }

    template<typename L, typename R>
    constexpr auto makeFormatAttributesInner(fmt::detail::concat<L, R> fmt) noexcept {
        return makeFormatAttributesInner(fmt.lhs) + makeFormatAttributesInner(fmt.rhs);
    }

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
    struct PartialAttributeArray {
        MessageAttributeInfo attributes[N] = {};
        int indices;

        constexpr size_t size() const noexcept { return N + indices; }
        constexpr std::span<const MessageAttributeInfo> namedAttributes() const noexcept { return { attributes, N }; }
    };

    template<size_t N>
    constexpr PartialAttributeArray<N> makePartialAttributeArray(std::span<const InnerAttributeInfo> attributes) noexcept {
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

    struct Result {
        std::array<char, 128> result;
        size_t size;

        constexpr char *data() noexcept { return result.data(); }
        constexpr const char *data() const noexcept { return result.data(); }
        constexpr void setSize(char *end) noexcept { size = end - result.data(); }

        constexpr operator std::string_view() const noexcept { return { result.data(), size }; }
    };

    template<typename T>
    constexpr auto cleanFormatString(T compiled) {
        Result result{};
        char *end = result.data() + result.result.size();

        char *dst = result.data();
        for (auto i = 0; i < compiled.indices; i++) {
            *dst++ = '{';
            dst = std::to_chars(dst, end, i).ptr;
            *dst++ = '}';
        }
        for (const auto& attr : compiled.namedAttributes()) {
            *dst++ = '{';
            for (const char c : attr.name) {
                *dst++ = c;
            }
            *dst++ = '}';
        }
        *dst = '\0';
        result.setSize(dst);
        return result;
    }
}

#define BUILD_MESSAGE_ATTRIBUTES_IMPL(message, ...) \
    []() constexpr { \
        using Wrapper = decltype(sm::logs::compileTupleArgs(__VA_ARGS__)); \
        return (Wrapper{})(FMT_COMPILE(message)); \
    }()


#if SMC_LOGS_INCLUDE_SOURCE_INFO
#   define BUILD_MESSAGE_IMPL(info, message) message
#else
#   define BUILD_MESSAGE_IMPL(info, message) sm::logs::cleanFormatString(info)
#endif
