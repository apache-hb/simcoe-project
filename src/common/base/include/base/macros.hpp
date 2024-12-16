#pragma once

#define SM_UNUSED [[maybe_unused]]

#define SM_COPY(CLS, IS) \
    CLS(const CLS &other) noexcept = IS; \
    CLS &operator=(const CLS &other) noexcept = IS;

#define SM_MOVE(CLS, IS) \
    CLS(CLS &&other) noexcept = IS; \
    CLS &operator=(CLS &&other) noexcept = IS;

#define SM_NOCOPY(CLS) SM_COPY(CLS, delete)
#define SM_NOMOVE(CLS) SM_MOVE(CLS, delete)

#define SM_CONCAT(a, b) a ## b

#define SM_SWAP_MOVE(CLS) \
    CLS(CLS &&other) noexcept { swap(*this, other); } \
    CLS &operator=(CLS &&other) noexcept { swap(*this, other); return *this; }

#define TRY(expr, ...) ({ auto innerResult = (expr); if (!innerResult) { [&] __VA_ARGS__ (innerResult.error()); return innerResult.error(); } std::move(innerResult.value()); })
#define TRY_RESULT(expr) ({ auto innerResult = (expr); if (!innerResult) return std::unexpected(innerResult.error()); std::move(innerResult.value()); })
#define TRY_UNWRAP(expr) ({ auto innerResult = (expr); if (!innerResult) return innerResult.error(); std::move(innerResult.value()); })
#define TRY_RETURN(expr, ...) ({ auto innerResult = (expr); if (!innerResult) { return [&] __VA_ARGS__ (innerResult.error()); } std::move(innerResult.value()); })

#if __cpp_deleted_function >= 202403L
#   define SM_DELETE_REASON(it) delete(it)
#else
#   define SM_DELETE_REASON(it) delete
#endif

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
