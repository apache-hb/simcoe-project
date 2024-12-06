#include "system/system.hpp"

#include "common.hpp"

#include "core/defer.hpp"

#include <fcntl.h>

#include <ranges>

using namespace sm;

static constexpr int kBufferSize = 512;

OsError system::getLastError() {
    return errno;
}

// TODO: test this
std::vector<std::string> system::getCommandLine() {
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1)
        throw OsException{OsError{os_error_t(errno)}, "open(\"/proc/self/cmdline\")"};

    defer { close(fd); };

    std::vector<std::string> args;

    char buffer[kBufferSize];
    std::string arg;
    while (true) {
        int size = read(fd, buffer, sizeof(buffer));
        if (size == 0 || size == -1)
            break; // done reading

        size_t offset = 0;
        while (offset < size_t(size)) {
            size_t len = strnlen(buffer + offset, size);
            if (len != size - offset) {
                args.emplace_back(arg);
                arg = "";
            }
            arg.append(std::string_view(buffer + offset, len));
            offset += len;
        }
    }

    args.emplace_back(arg);

    return args;
}

os_error_t system::getMachineId(char buffer[kMachineIdSize]) noexcept {
    int fd = open("/etc/machine-id", O_RDONLY);
    if (fd == -1) {
        memset(buffer, '0', kMachineIdSize);
        return errno;
    }

    defer { close(fd); }

    int size = read(fd, buffer, kMachineIdSize);
    if (size < kMachineIdSize) {
        memset(buffer + size, '-', kMachineIdSize - size);
    }

    return 0;
}

void system::create(HINSTANCE hInstance) {
    initStorage();
}

void system::destroy(void) noexcept {
    // empty for now
}
