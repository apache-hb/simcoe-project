#pragma once

#ifdef _MSC_VER
#   define SM_CC_MSVC
#elif defined(__GNUC__)
#   define SM_CC_GCC
#elif defined(__clang__)
#   define SM_CC_CLANG
#else
#   error "Unsupported compiler"
#endif

#ifdef _WIN32
#   define SM_OS_WINDOWS
#elif defined(__linux__)
#   define SM_OS_LINUX
#elif defined(__APPLE__)
#   define SM_OS_MACOS
#else
#   error "Unsupported OS"
#endif

#if defined(_MSC_VER)
#   define SM_PRIVATE
#   define SM_EXPORT __declspec(dllexport)
#   define SM_IMPORT __declspec(dllimport)
#elif defined(__GNUC__)
#   if __GNUC__ >= 4
#       define SM_PRIVATE __attribute__((visibility("hidden")))
#       define SM_EXPORT __attribute__((visibility("default")))
#       define SM_IMPORT
#   else
#       define SM_PRIVATE
#       define SM_EXPORT
#       define SM_IMPORT
#   endif
#endif
