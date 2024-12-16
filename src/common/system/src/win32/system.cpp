#include "system/system.hpp"
#include "base/defer.hpp"
#include "core/string.hpp"

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

os_error_t system::getMachineId(char buffer[kMachineIdSize]) noexcept {
    const char *key = "SOFTWARE\\Microsoft\\Cryptography";
    const char *value = "MachineGuid";

    DWORD size = kMachineIdSize;
    if (LSTATUS status = RegGetValueA(HKEY_LOCAL_MACHINE, key, value, RRF_RT_REG_SZ, nullptr, buffer, &size)) {
        memcpy(buffer, "00000000-0000-0000-0000-000000000000", kMachineIdSize);
        return status;
    }

    return ERROR_SUCCESS;
}
