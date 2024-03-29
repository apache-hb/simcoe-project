#pragma once

#define META_CONCAT_IMPL(x, y) x##y
#define META_CONCAT(x, y) META_CONCAT_IMPL(x, y)

#if defined(__clang__)
#   define META_ANNOTATE(...) [[clang::annotate(__VA_ARGS__)]]
#else
#   define META_ANNOTATE(...)
#endif

#if defined(__REFLECT__)
#   define REFLECT(...) [meta_class(__VA_ARGS__)]
#   define INTERFACE(...) [meta_interface(__VA_ARGS__)]
#   define ENUM(...) [meta_enum(__VA_ARGS__)]
#   define PROPERTY(...) [[simcoe::meta_property(__VA_ARGS__)]]
#   define METHOD(...) [[simcoe::meta_method(__VA_ARGS__)]]
#   define CASE(...) [meta_case(__VA_ARGS__)]
#   define REFLECT_BODY(ID)
#   include "tags.hpp"
#else
#   define REFLECT(...)
#   define INTERFACE(...)
#   define ENUM(...)
#   define PROPERTY(...)
#   define METHOD(...)
#   define CASE(...)
#   define REFLECT_BODY(ID) META_CONCAT(ID, _REFLECT)
#endif

namespace sm::meta {
    class META_ANNOTATE("meta_object") Object {

    };
}
