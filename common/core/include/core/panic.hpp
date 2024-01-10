#pragma once

#include <core_api.hpp>

#include <string>

namespace simcoe {
    struct PanicInfo {
        const char *file;
        const char *symbol;
        size_t line;
    };

    [[noreturn]]
    SM_CORE_API void panic(const PanicInfo &info, std::string msg);
}

#define SM_ASSERT_ALWAYS(cond) \
    do { \
        if (!(cond)) { \
            simcoe::PanicInfo panic_info { \
                .file = __FILE__, \
                .symbol = __func__, \
                .line = __LINE__ \
            }; \
            simcoe::panic(panic_info, #cond); \
        } \
    } while (0)

#if SMC_DEBUG
#   define SM_ASSERT(cond) SM_ASSERT_ALWAYS(cond)
#else
#   define SM_ASSERT(cond) do { } while (0)
#endif
