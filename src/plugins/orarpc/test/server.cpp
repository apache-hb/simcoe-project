#include "core/defer.hpp"
#include "core/error.hpp"
#include "launch/launch.hpp"

#include "net/net.hpp"
#include <csignal>
#include <ranges>
#include <sys/stat.h>

using namespace sm;

using namespace std::chrono_literals;

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "test-orarpc-logs.db" },
    .logPath = "test-orarpc.log",

    .threads = false,
    .network = true,
    .com = false,
    .allowInvalidArgs = true,
};

static std::vector<std::string> getCommandLine() {
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1)
        return {};

    defer { close(fd); };

    char buffer[0x1000];
    int size = read(fd, buffer, sizeof(buffer));

    std::vector<std::string> args;

    for (const auto& part : std::views::split(std::string_view(buffer, size), '\0')) {
        args.push_back(std::string(part.begin(), part.end()));
    }

    return args;
}

static std::string getAt(const std::vector<std::string>& range, size_t index) {
    if (range.size() <= index) {
        return "";
    }

    return range[index];
}

static const char *kPidFile = "/run/ons.pid";

static int getDaemonPid() {
    std::ifstream file(kPidFile);
    if (!file.is_open()) {
        LOG_WARN(GlobalLog, "failed to open pid file");
        return -1;
    }

    int pid = 0;
    file >> pid;
    return pid;
}

static bool isDaemonRunning(int pid) {
    // test if the process is still running
    return std::filesystem::exists(fmt::format("/proc/{}", pid));
}

static int performPing() {
    int pid = getDaemonPid();
    if (isDaemonRunning(pid)) {
        LOG_INFO(GlobalLog, "daemon is running. pid {}", pid);
        return 0;
    } else {
        LOG_WARN(GlobalLog, "daemon is not running. pid {}", pid);
        return 1;
    }
}

static int killDaemon() {
    int pid = getDaemonPid();
    if (pid == -1) {
        LOG_ERROR(GlobalLog, "daemon is not running");
        return 1;
    }

    if (kill(pid, SIGTERM) == -1) {
        LOG_ERROR(GlobalLog, "failed to kill daemon {}: {}", pid, OsError(errno));
        return 1;
    }

    std::filesystem::remove(kPidFile);

    return 0;
}

static void writePidFile() {
    std::filesystem::remove(kPidFile);

    std::ofstream file(kPidFile);
    if (!file.is_open()) {
        throw OsException(OsError(errno), "failed to open pid file");
    }

    file << getpid() << '\n';
}

int commonMain() noexcept try {
    auto args = getCommandLine();
    if (args.size() < 2) {
        LOG_ERROR(GlobalLog, "Failed to read command line");
        return -1;
    }

    // TODO: whats the difference between ping & pingwait?
    if (getAt(args, 1) == "-a") {
        if (getAt(args, 2) == "ping") {
            return performPing();
        } else if (getAt(args, 2) == "pingwait") {
            return performPing();
        } else if (getAt(args, 2) == "shutdown") {
            return killDaemon();
        }
    }

    // daemonize
    if (getAt(args, 1) == "-d") {
        daemon(0, 0);
        writePidFile();
    }

    net::Network network = net::Network::create();
    // bind to the ons port 6200
    net::ListenSocket server = network.bind(net::Address::of("0.0.0.0"), 6200);

    server.listen(net::ListenSocket::kMaxBacklog).throwIfFailed();

    int i = 0;
    while (true) {
        auto client = server.accept();
        client.setBlocking(false).throwIfFailed();
        client.setRecvTimeout(250ms).throwIfFailed();

        std::ofstream file(fmt::format("/opt/shared/messages/client_{:0>8}.bin", i++));

        char buffer[1024];
        while (true) {
            auto [read, err] = client.recvBytesTimeout(buffer, sizeof(buffer), 250ms);
            file.write(buffer, read);

            if (err.timeout() && read == 0) {
                LOG_ERROR(GlobalLog, "client error: {}", err);
                break;
            }
        }
    }

    return 0;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}

SM_LAUNCH_MAIN("ORARPC", commonMain, kLaunchInfo)
