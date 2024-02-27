#pragma once

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
