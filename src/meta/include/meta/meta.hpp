#pragma once

#if defined(__REFLECT__)
#   define reflect(...) [[clang::annotate("reflect@" #__VA_ARGS__)]]
#   define reflect_impl()
#   define reflect_header(...) "meta/empty.hpp"
#else
#   define reflect(...)
#   define reflect_impl()
#   define reflect_header(...) #__VA_ARGS__
#endif
