#pragma once

#ifdef __REFLECT__
#   define REFLECT(...) [simcoe_meta(__VA_ARGS__)]
#   define REFLECT_BODY(id)
#   define REFLECT_ENUM(id, ...) REFLECT(__VA_ARGS__)
#   define REFLECT_EXTERNAL_ENUM(id, ...) REFLECT(__VA_ARGS__) constexpr id __reflect_##id{};
#else
#   define REFLECT(...)
#   define REFLECT_BODY(id) REFLECT_IMPL_##id
#   define REFLECT_ENUM(id, ...) REFLECT_BODY(id)
#   define REFLECT_EXTERNAL_ENUM(id, ...)
#endif

#define CASE(...) REFLECT(__VA_ARGS__)
#define FIELD(...) REFLECT(__VA_ARGS__)
#define METHOD(...) REFLECT(__VA_ARGS__)

#include "tags.hpp"
#include "class.hpp"
