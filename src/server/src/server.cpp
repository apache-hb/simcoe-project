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
        server.listen(16);

        std::stop_callback cb(stop, [&] {
            LOG_INFO(ServerLog, "Stopping server");
            server.stop();
        });
    });

    if (gRunMode.getValue() == RunMode::eGui) {
        GuiWindow window{"Server"};
        while (window.next()) {
            window.present();
        }
    } else {
        while (true) std::this_thread::sleep_for(1s);
    }

    return 0;
}

static int commonMain() noexcept try {
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
};

int main(int argc, const char **argv) noexcept try {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    sm::Span<const char*> args{argv, size_t(argc)};
    LOG_INFO(GlobalLog, "args = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(argc, argv)) {
        return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
    }

    int result = commonMain();

    LOG_INFO(ServerLog, "editor exiting with {}", result);

    return result;
} catch (const db::DbException& err) {
    LOG_ERROR(ServerLog, "database error: {}", err.error());
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(ServerLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(ServerLog, "unknown unhandled exception");
    return -1;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    LOG_INFO(ServerLog, "lpCmdLine = {}", lpCmdLine);
    LOG_INFO(ServerLog, "nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = commonMain();

    LOG_INFO(ServerLog, "editor exiting with {}", result);

    return result;
}
