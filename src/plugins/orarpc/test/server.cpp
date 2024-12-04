
#include "launch/launch.hpp"

#include "fmt/ranges.h"

#include "net/net.hpp"

#include "core/defer.hpp"
#include "core/error.hpp"

#include "orarpc/ssl.hpp"

#include "system/system.hpp"

#include <csignal>
#include <sys/stat.h>

using namespace sm;

using namespace std::chrono_literals;

#define PID_FILE_PATH "/run/ons.pid"
#define DAEOMON_DIR "/run/ons.d"
#define PRIVATE_KEY_PATH DAEOMON_DIR "/ons.key"
#define CERTIFICATE_PATH DAEOMON_DIR "/ons.pem"

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "/test-orarpc-logs.db" },
    .logPath = "/test-orarpc.log",

    .threads = false,
    .network = true,
    .com = false,
    .allowInvalidArgs = true,
};

static std::string getAt(const std::vector<std::string>& range, size_t index) {
    if (range.size() <= index) {
        return "";
    }

    return range[index];
}

static int getDaemonPid() {
    std::ifstream file(PID_FILE_PATH);
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

    std::filesystem::remove(PID_FILE_PATH);

    return 0;
}

static void writePidFile() {
    std::filesystem::remove(PID_FILE_PATH);

    std::ofstream file(PID_FILE_PATH);
    if (!file.is_open()) {
        throw OsException(OsError(errno), "failed to open pid file");
    }

    file << getpid() << '\n';
}

static ssl::PrivateKey loadPrivateKey() {
    if (fs::is_regular_file(PRIVATE_KEY_PATH)) {
        return ssl::PrivateKey::open(PRIVATE_KEY_PATH);
    }

    ssl::PrivateKey key(2048);
    std::ofstream file(PRIVATE_KEY_PATH);
    key.save(file);

    return key;
}

static ssl::X509Certificate loadCertificate(ssl::PrivateKey& key) {
    if (fs::is_regular_file(CERTIFICATE_PATH)) {
        return ssl::X509Certificate::open(CERTIFICATE_PATH);
    }

    ssl::X509Certificate cert(key);
    std::ofstream file(CERTIFICATE_PATH);
    cert.save(file);

    return cert;
}

int commonMain(launch::LaunchResult&) noexcept try {
    auto args = system::getCommandLine();
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

    auto prepareDir = [&](const char *path) {
        std::error_code ec;
        if (!fs::create_directories(path, ec) && ec != std::errc::file_exists && ec) {
            LOG_ERROR(GlobalLog, "failed to create directory {}: {}", path, ec.message());
            return false;
        }

        return true;
    };

    prepareDir(DAEOMON_DIR);
    prepareDir("/opt/shared/messages");
    prepareDir("/opt/shared/certs");
    prepareDir("/opt/shared/logs");

    ssl::PrivateKey key = loadPrivateKey();
    ssl::X509Certificate cert = loadCertificate(key);

    ssl::SslContext sslContext{ std::move(key), std::move(cert) };

    LOG_INFO(GlobalLog, "Successfully loaded ssl context");

    int i = 0;
    while (true) {
        std::ofstream logFile(fmt::format("/opt/shared/logs/server_{:0>8}.log", i++));
        ssl::SslSession session(sslContext, server.accept());
        std::ofstream messageFile(fmt::format("/opt/shared/messages/client_{:0>8}.bin", i));

        logFile << "Client connected\n";
        LOG_INFO(GlobalLog, "Client {} connected", i);

        if (ssl::SslError error = session.accept()) {
            logFile << "Client failed to connect: " << fmt::to_string(error) << '\n';
            LOG_INFO(GlobalLog, "Client {} failed to connect: {}", i, error);
            continue;
        }

        std::ofstream certFile(fmt::format("/opt/shared/certs/client_{:0>8}.pem", i));

        if (X509 *cert = session.getPeerCertificate()) {
            logFile << "Client presented certificate\n";
            ssl::X509Certificate::save(certFile, cert);
        } else {
            logFile << "Client presented no certificate\n";
            LOG_WARN(GlobalLog, "Client {} presented no certificate", i);
        }

        char buffer[1024];
        while (true) {
            int length = session.readBytes(buffer, sizeof(buffer));
            if (length == -1) {
                LOG_ERROR(GlobalLog, "Client {} read error: {}", i, ssl::SslError::errorOf("SSL_read"));
                break;
            }

            if (length == 0) {
                break;
            }

            messageFile.write(buffer, length);
        }

        logFile << "Client disconnected\n";

        LOG_INFO(GlobalLog, "Client {} disconnected", i);
    }

#if 0
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
#endif
    return 0;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}

SM_LAUNCH_MAIN("ORARPC", commonMain, kLaunchInfo)
