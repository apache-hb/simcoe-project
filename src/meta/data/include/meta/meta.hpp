#pragma once

#if defined(__REFLECT__)
#   define REFLECT(...) [meta(__VA_ARGS__)]
#   define INTERFACE(...) [meta_interface(__VA_ARGS__)]
#   define PROPERTY(...) [[simcoe::meta_property(__VA_ARGS__)]]
#   define METHOD(...) [[simcoe::meta_method(__VA_ARGS__)]]
#   define REFLECT_BODY()
#   include "tags.hpp"
#else
#   define REFLECT(...)
#   define INTERFACE(...)
#   define PROPERTY(...)
#   define METHOD(...)
#   define REFLECT_BODY()
#endif
