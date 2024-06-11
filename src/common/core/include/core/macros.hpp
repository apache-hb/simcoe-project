#pragma once

#include <string_view>

#define SM_UNUSED [[maybe_unused]]

#define SM_COPY(CLS, IS) \
    CLS(const CLS &other) = IS; \
    CLS &operator=(const CLS &other) = IS;

#define SM_MOVE(CLS, IS) \
    CLS(CLS &&other) = IS; \
    CLS &operator=(CLS &&other) = IS;

#define SM_NOCOPY(CLS) SM_COPY(CLS, delete)
#define SM_NOMOVE(CLS) SM_MOVE(CLS, delete)

#if defined(_MSC_VER)
#   if defined(__clang__)
#      if __clang_major__ >= 18
#         define SM_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#      endif
#   elif _MSC_VER >= 1910
#      define SM_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#   endif
#endif

#ifndef SM_NO_UNIQUE_ADDRESS
#   define SM_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

// borrowed from flecs (who took it from https://blog.molecular-matters.com/2015/12/11/getting-the-type-of-a-template-argument-as-string-without-rtti/)
#if defined(__clang__)
#   define SM_FN_NAME_PREFIX(type, name) ((sizeof(#type) + sizeof(" sm::() [T = ") + sizeof(#name)) - 3u)
#   define SM_FN_NAME_SUFFIX (sizeof("]") - 1u)
#   define SM_FN_NAME __PRETTY_FUNCTION__
#elif defined(__GNUC__)
#   define SM_FN_NAME_PREFIX(type, name) ((sizeof(#type) + sizeof(" sm::() [with T = ") + sizeof(#name)) - 3u)
#   define SM_FN_NAME_SUFFIX (sizeof("]") - 1u)
#   define SM_FN_NAME __PRETTY_FUNCTION__
#elif defined(_WIN32)
#   define SM_FN_NAME_PREFIX(type, name) ((sizeof(#type) + sizeof(" __cdecl sm::<") + sizeof(#name)) - 3u)
#   define SM_FN_NAME_SUFFIX (sizeof(">(void)") - 1u)
#   define SM_FN_NAME __FUNCSIG__
#else
#   error "Unsupported compiler"
#endif

#define SM_FN_NAME_LENGTH(type, name, str)\
    (sizeof(str) - (SM_FN_NAME_PREFIX(type, name) + SM_FN_NAME_SUFFIX))

namespace sm {
    std::string_view trimTypeName(std::string_view name);
    size_t cleanTypeName(char *dst, std::string_view name);

    template<typename T>
    std::string_view computeTypeName() {
        static constexpr std::string_view kName = SM_FN_NAME;
        static constexpr size_t kLength = SM_FN_NAME_LENGTH(std::string_view, computeTypeName, SM_FN_NAME);
        static constexpr size_t kPrefixLength = SM_FN_NAME_PREFIX(std::string_view, computeTypeName);
        static const std::string_view kTrimmed = trimTypeName(kName.substr(kPrefixLength, kLength - kPrefixLength - SM_FN_NAME_SUFFIX));
        static char name[kLength + 1] = {0};

        size_t used = cleanTypeName(name, kTrimmed);

        return std::string_view(name, used);
    }

    template<typename T>
    std::string_view getTypeName() {
        static std::string_view name = computeTypeName<T>();
        return name;
    }
}
