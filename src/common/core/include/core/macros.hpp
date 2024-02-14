#pragma once

#define SM_UNUSED [[maybe_unused]]

#define SM_NOCOPY(CLS)         \
    CLS(const CLS &) = delete; \
    CLS &operator=(const CLS &) = delete;

#define SM_NOMOVE(CLS)    \
    CLS(CLS &&) = delete; \
    CLS &operator=(CLS &&) = delete;

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
#   define SM_NO_UNIQUE_ADDRESS
#endif
