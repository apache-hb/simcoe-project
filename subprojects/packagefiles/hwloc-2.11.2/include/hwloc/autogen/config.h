#include "hwloc_autogen_config.h"


#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
# define __hwloc_restrict __restrict
#else
# if __STDC_VERSION__ >= 199901L
#  define __hwloc_restrict restrict
# else
#  define __hwloc_restrict
# endif
#endif

/* Note that if we're compiling C++, then just use the "inline"
   keyword, since it's part of C++ */
#if defined(c_plusplus) || defined(__cplusplus)
#  define __hwloc_inline inline
#elif defined(_MSC_VER) || defined(__HP_cc)
#  define __hwloc_inline __inline
#else
#  define __hwloc_inline __inline__
#endif

/*
 * Note: this is public.  We can not assume anything from the compiler used
 * by the application and thus the HWLOC_HAVE_* macros below are not
 * fetched from the autoconf result here. We only automatically use a few
 * well-known easy cases.
 */

/* Some handy constants to make the logic below a little more readable */
#if defined(__cplusplus) && \
    (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR >= 4))
#define GXX_ABOVE_3_4 1
#else
#define GXX_ABOVE_3_4 0
#endif

#if !defined(__cplusplus) && \
    (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
#define GCC_ABOVE_2_95 1
#else
#define GCC_ABOVE_2_95 0
#endif

#if !defined(__cplusplus) && \
    (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96))
#define GCC_ABOVE_2_96 1
#else
#define GCC_ABOVE_2_96 0
#endif

#if !defined(__cplusplus) && \
    (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#define GCC_ABOVE_3_3 1
#else
#define GCC_ABOVE_3_3 0
#endif

#if !defined(__cplusplus) &&					\
    (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#define GCC_ABOVE_3_4 1
#else
#define GCC_ABOVE_3_4 0
#endif

/* Maybe before gcc 2.95 too */
#ifdef HWLOC_HAVE_ATTRIBUTE_UNUSED
#define __HWLOC_HAVE_ATTRIBUTE_UNUSED HWLOC_HAVE_ATTRIBUTE_UNUSED
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_UNUSED (GXX_ABOVE_3_4 || GCC_ABOVE_2_95)
#else
# define __HWLOC_HAVE_ATTRIBUTE_UNUSED 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_UNUSED
# define __hwloc_attribute_unused __attribute__((__unused__))
#else
# define __hwloc_attribute_unused
#endif

#ifdef HWLOC_HAVE_ATTRIBUTE_MALLOC
#define __HWLOC_HAVE_ATTRIBUTE_MALLOC HWLOC_HAVE_ATTRIBUTE_MALLOC
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_MALLOC (GXX_ABOVE_3_4 || GCC_ABOVE_2_96)
#else
# define __HWLOC_HAVE_ATTRIBUTE_MALLOC 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_MALLOC
# define __hwloc_attribute_malloc __attribute__((__malloc__))
#else
# define __hwloc_attribute_malloc
#endif

#ifdef HWLOC_HAVE_ATTRIBUTE_CONST
#define __HWLOC_HAVE_ATTRIBUTE_CONST HWLOC_HAVE_ATTRIBUTE_CONST
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_CONST (GXX_ABOVE_3_4 || GCC_ABOVE_2_95)
#else
# define __HWLOC_HAVE_ATTRIBUTE_CONST 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_CONST
# define __hwloc_attribute_const __attribute__((__const__))
#else
# define __hwloc_attribute_const
#endif

#ifdef HWLOC_HAVE_ATTRIBUTE_PURE
#define __HWLOC_HAVE_ATTRIBUTE_PURE HWLOC_HAVE_ATTRIBUTE_PURE
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_PURE (GXX_ABOVE_3_4 || GCC_ABOVE_2_96)
#else
# define __HWLOC_HAVE_ATTRIBUTE_PURE 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_PURE
# define __hwloc_attribute_pure __attribute__((__pure__))
#else
# define __hwloc_attribute_pure
#endif

#ifndef __hwloc_attribute_deprecated /* allow the user to disable these warnings by defining this macro to nothing */
#ifdef HWLOC_HAVE_ATTRIBUTE_DEPRECATED
#define __HWLOC_HAVE_ATTRIBUTE_DEPRECATED HWLOC_HAVE_ATTRIBUTE_DEPRECATED
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_DEPRECATED (GXX_ABOVE_3_4 || GCC_ABOVE_3_3)
#else
# define __HWLOC_HAVE_ATTRIBUTE_DEPRECATED 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_DEPRECATED
# define __hwloc_attribute_deprecated __attribute__((__deprecated__))
#else
# define __hwloc_attribute_deprecated
#endif
#endif

#ifdef HWLOC_HAVE_ATTRIBUTE_MAY_ALIAS
#define __HWLOC_HAVE_ATTRIBUTE_MAY_ALIAS HWLOC_HAVE_ATTRIBUTE_MAY_ALIAS
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_MAY_ALIAS (GXX_ABOVE_3_4 || GCC_ABOVE_3_3)
#else
# define __HWLOC_HAVE_ATTRIBUTE_MAY_ALIAS 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_MAY_ALIAS
# define __hwloc_attribute_may_alias __attribute__((__may_alias__))
#else
# define __hwloc_attribute_may_alias
#endif

#ifdef HWLOC_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT
#define __HWLOC_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT HWLOC_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT
#elif defined(__GNUC__)
# define __HWLOC_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT (GXX_ABOVE_3_4 || GCC_ABOVE_3_4)
#else
# define __HWLOC_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT 0
#endif
#if __HWLOC_HAVE_ATTRIBUTE_WARN_UNUSED_RESULT
# define __hwloc_attribute_warn_unused_result __attribute__((__warn_unused_result__))
#else
# define __hwloc_attribute_warn_unused_result
#endif

#ifdef HWLOC_C_HAVE_VISIBILITY
# if HWLOC_C_HAVE_VISIBILITY
#  define HWLOC_DECLSPEC __attribute__((__visibility__("default")))
# else
#  define HWLOC_DECLSPEC
# endif
#else
# define HWLOC_DECLSPEC
#endif


#ifdef HWLOC_HAVE_WINDOWS_H

#  include <windows.h>
typedef DWORDLONG hwloc_uint64_t;

#else /* HWLOC_HAVE_WINDOWS_H */

#  ifdef hwloc_thread_t
#    include <pthread.h>
#  endif /* hwloc_thread_t */

#  include <unistd.h>
#  include <stdint.h>

typedef uint64_t hwloc_uint64_t;

#endif /* HWLOC_HAVE_WINDOWS_H */
