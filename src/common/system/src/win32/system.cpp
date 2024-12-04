#include "system/system.hpp"
#include "core/defer.hpp"

#include <shellapi.h>

using namespace sm;

OsError system::getLastError() {
    return GetLastError();
}

std::vector<std::string> system::getCommandLine() {
    int argc = 0;
    if (LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc)) {
        defer { LocalFree((void*)argvw); };

        std::vector<std::string> args;

        args.reserve(args.size() + argc);
        for (int i = 0; i < argc; ++i) {
            args.push_back(sm::narrow(argvw[i]));
        }

        return args;
    }

    throw OsException{OsError{GetLastError()}, "CommandLineToArgvW"};
}

std::string system::getMachineId() {
    const char *key = "SOFTWARE\\Microsoft\\Cryptography";
    const char *value = "MachineGuid";

    char buffer[256];
    DWORD size = sizeof(buffer);
    if (RegGetValueA(HKEY_LOCAL_MACHINE, key, value, RRF_RT_REG_SZ, nullptr, buffer, &size) != ERROR_SUCCESS) {
        return "00000000-0000-0000-0000-000000000000";
    }

    return buffer;
}
