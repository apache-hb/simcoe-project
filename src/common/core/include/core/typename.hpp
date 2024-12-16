#pragma once

#include <string_view>

// borrowed from flecs (who took it from https://blog.molecular-matters.com/2015/12/11/getting-the-type-of-a-template-argument-as-string-without-rtti/)
#if defined(__clang__)
#   define SM_FN_NAME_PREFIX(type, name) ((sizeof(#type) + sizeof(" sm::() [T = ") + sizeof(#name)) - 3u)
#   define SM_FN_NAME_SUFFIX (sizeof("]") - 1u)
#   define SM_FN_NAME __PRETTY_FUNCTION__
#elif defined(__GNUC__)
#   define SM_FN_NAME_PREFIX(type, name) ((sizeof(#type) + sizeof(" sm::() [with T = ") + sizeof(#name)) - 3u)
#   define SM_FN_NAME_SUFFIX (sizeof("]") - 1u)
#   define SM_FN_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#   define SM_FN_NAME_PREFIX(type, name) ((sizeof(#type) + sizeof(" __cdecl sm::<") + sizeof(#name)) - 3u)
#   define SM_FN_NAME_SUFFIX (sizeof(">(void)") - 1u)
#   define SM_FN_NAME __FUNCSIG__
#else
#   error "Unsupported compiler"
#endif

#define SM_FN_NAME_LENGTH(type, name, str) \
    (sizeof(str) - (SM_FN_NAME_PREFIX(type, name) + SM_FN_NAME_SUFFIX))

namespace sm {
    std::string_view trimTypeName(std::string_view name);
    size_t cleanTypeName(char *dst, std::string_view name);

    // need to be explicit about std::basic_string_view<char> to prevent
    // gcc adding `std::string_view = std::basic_string_view<char>`
    // to the mangled symbol name
    template<typename T>
    std::basic_string_view<char> computeTypeName() {
        constexpr std::string_view kName = SM_FN_NAME;
        constexpr size_t kLength = SM_FN_NAME_LENGTH(std::basic_string_view<char>, computeTypeName, SM_FN_NAME);
        constexpr size_t kPrefixLength = SM_FN_NAME_PREFIX(std::basic_string_view<char>, computeTypeName);
        const std::string_view kTrimmed = trimTypeName(kName.substr(kPrefixLength, kLength));
        static char name[kLength + 1];

        size_t used = cleanTypeName(name, kTrimmed);

        return std::string_view(name, used);
    }

    template<typename T>
    std::string_view getTypeName() {
        static std::string_view name = computeTypeName<T>();
        return name;
    }
}
