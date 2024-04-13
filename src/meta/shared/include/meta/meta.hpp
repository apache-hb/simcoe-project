#pragma once

#define META_CONCAT_IMPL(x, y) x##y
#define META_CONCAT(x, y) META_CONCAT_IMPL(x, y)

#if defined(__clang__)
#   define META_ANNOTATE(...) [[clang::annotate(__VA_ARGS__)]]
#else
#   define META_ANNOTATE(...)
#endif

#undef INTERFACE

#if defined(__REFLECT__)
namespace sm::meta::detail {
    struct Empty { };
}
#   define REFLECT(...) [meta_class(__VA_ARGS__)]
#   define INTERFACE(...) [meta_interface(__VA_ARGS__)]
#   define ENUM(...) [meta_enum(__VA_ARGS__)]
#   define PROPERTY(...) [[simcoe::meta_property(__VA_ARGS__)]]
#   define METHOD(...) [[simcoe::meta_method(__VA_ARGS__)]]
#   define CASE(...) [meta_case(__VA_ARGS__)]
#   define REFLECT_BODY(ID) [[msvc::no_unique_address]] sm::meta::detail::Empty __reflect_body;
#   include "tags.hpp"
using namespace sm::meta::tags;
#else
#   define REFLECT(...)
#   define INTERFACE(...)
#   define ENUM(...)
#   define PROPERTY(...)
#   define METHOD(...)
#   define CASE(...)
#   define REFLECT_BODY(ID) META_CONCAT(ID, __reflect_body)
#endif
