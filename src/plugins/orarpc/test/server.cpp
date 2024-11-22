#include "launch/launch.hpp"

#include "net/net.hpp"

using namespace sm;

using namespace std::chrono_literals;

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "test-orarpc-logs.db" },
    .logPath = "test-orarpc.log",

    .threads = false,
    .network = true,
    .com = false,
};

int commonMain() noexcept try {
    LOG_INFO(GlobalLog, "Success");

    net::Network network = net::Network::create();
    // the ologgerd service binds to 21455
    net::ListenSocket server = network.bind(net::Address::of("0.0.0.0"), 21455);

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
            if (err) {
                LOG_ERROR(GlobalLog, "client error: {}", err);
                break;
            }

            file.write(buffer, read);
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
