#include "system/system.hpp"

#include "core/defer.hpp"

#include <fcntl.h>

#include <ranges>

using namespace sm;

std::vector<std::string> system::getCommandLine() {
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1)
        throw OsException{OsError{os_error_t(errno)}, "open /proc/self/cmdline"};

    defer { close(fd); };

    std::vector<std::string> args;

    char buffer[0x1000];
    int size = read(fd, buffer, sizeof(buffer));

    // TODO: handle large command lines

    for (const auto& part : std::views::split(std::string_view(buffer, size), '\0')) {
        args.push_back(std::string(part.begin(), part.end()));
    }

    return args;
}
