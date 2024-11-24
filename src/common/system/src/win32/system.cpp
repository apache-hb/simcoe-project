#include "system/system.hpp"
#include "core/defer.hpp"

#include <shellapi.h>

using namespace sm;

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
