#pragma once

#if defined(__REFLECT__)
#   define REFLECTED(...) [meta(#__VA_ARGS__)]
#   define PROPERTY(...) [[simcoe::meta_property(#__VA_ARGS__)]]
#   define METHOD(...) [[simcoe::meta_method(#__VA_ARGS__)]]
#   define REFLECT_BODY()
#else
#   define REFLECTED(...)
#   define PROPERTY(...)
#   define METHOD(...)
#   define REFLECT_BODY()
#endif
