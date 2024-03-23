#pragma once

#if defined(__REFLECT__)
#   define reflect(...) [simcoe::meta(#__VA_ARGS__)]
#   define property(...) [[simcoe::meta_property(#__VA_ARGS__)]]
#else
#   define reflect(...)
#   define property(...)
#endif
