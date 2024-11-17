#pragma once

#include <string>
#include <string_view>
#include <experimental/generator>

#include <wil/wrl.h>

namespace sm::system {
    wil::unique_hkey openRegistryKey(HKEY hKey, const char *name);
    wil::unique_hkey openRegistryKey(HKEY hKey, const std::string& name);
    wil::unique_hkey tryOpenRegistryKey(HKEY hKey, const char *name);
}
