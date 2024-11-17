#include "system/registry.hpp"

#include "core/error.hpp"

using namespace sm;

wil::unique_hkey sm::system::openRegistryKey(HKEY hKey, const char *name) {
    HKEY hSubKey = nullptr;
    if (OsError error = RegOpenKeyExA(hKey, name, 0, KEY_READ, &hSubKey)) {
        throw OsException{error, "RegOpenKeyExW"};
    }

    return wil::unique_hkey{hSubKey};
}

wil::unique_hkey sm::system::openRegistryKey(HKEY hKey, const std::string& name) {
    return system::openRegistryKey(hKey, name.c_str());
}

wil::unique_hkey sm::system::tryOpenRegistryKey(HKEY hKey, const char *name) {
    HKEY hSubKey = nullptr;
    if (OsError error = RegOpenKeyExA(hKey, name, 0, KEY_READ, &hSubKey)) {
        return wil::unique_hkey{};
    }

    return wil::unique_hkey{hSubKey};
}
