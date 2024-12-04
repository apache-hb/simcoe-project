#include "stdafx.hpp"
#include "common.hpp"

#include "db/environment.hpp"
#include "db/connection.hpp"

#include "net/net.hpp"

#include "launch/launch.hpp"

#include "account/account.hpp"

using namespace sm;

using namespace std::chrono_literals;

LOG_MESSAGE_CATEGORY(ServerLog, "Server");

enum class RunMode {
    eGui,
    eHeadless,
};

static sm::opt<RunMode> gRunMode {
    name = "mode",
    choices = {
        val(RunMode::eGui) = "gui",
        val(RunMode::eHeadless) = "headless",
    }
};

static sm::opt<uint16_t> gServerPort {
    name = "port",
    init = 9919
};

static void serverGui(game::AccountServer &server) {
    launch::GuiWindow window{"Server"};
    while (!window.shouldClose()) {
        window.begin();

        window.end();
    }
}

static int serverMain() {
    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    net::Network network = net::Network::create();
    net::Address address = net::Address::any();

    game::AccountServer server {
        sqlite.connect({ .host = "server-users.db" }),
        network,
        address, gServerPort.getValue()
    };

    std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
        server.begin(16);

        std::stop_callback cb(stop, [&] {
            LOG_INFO(ServerLog, "Stopping server");
            server.stop();
        });

        server.work();
    });

    serverGui(server);

    return 0;
}

static int commonMain(launch::LaunchResult&) noexcept try {
    LOG_INFO(GlobalLog, "SMC_DEBUG = {}", SMC_DEBUG);
    LOG_INFO(GlobalLog, "CTU_DEBUG = {}", CTU_DEBUG);

    return serverMain();
} catch (const std::exception& err) {
    LOG_ERROR(ServerLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(ServerLog, "unknown unhandled exception");
    return -1;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "server-logs.db" },
    .logPath = "server.log",

    .network = true,
    .glfw = true,
};

SM_LAUNCH_MAIN("Server", commonMain, kLaunchInfo)
