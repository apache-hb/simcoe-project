#include "account_test_common.hpp"

#include "account/account.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr int kClientCount = 20;

namespace acd = sm::dao::account;

TEST_CASE("Account Create & Login") {
    net::create();

    TestServerConfig test{"account/lobby_list"};

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server = test.server(kAddress, 0, 1234);
        uint16_t port = server.getPort();

        auto serverThread = test.run(server, errors, kClientCount);

        // create clients
        createTestAccounts(test.network, kAddress, port, errors, kClientCount);


        // login with the created accounts but dont close connections
        std::latch sessionLatch{kClientCount + 1};
        std::latch alert{kClientCount + 1};

        std::jthread queryThread = std::jthread([&](const std::stop_token& stop) {
            game::AccountClient client { test.network, kAddress, port };

            errors.expect(client.createAccount("scanner", "password"), "failed to create scanner");
            errors.expect(client.login("scanner", "password"), "failed to login to scanner");

            alert.arrive_and_wait();

            // get all sessions

            client.refreshSessionList();

            auto sessions = client.getSessionInfo();

            errors.expect(sessions.size() >= kClientCount, "Expected at least {} sessions, got {}", kClientCount, sessions.size());

            /// get all lobbies

            client.refreshLobbyList();

            auto lobbies = client.getLobbyInfo();

            errors.expect(lobbies.size() >= kClientCount, "Expected 1 lobby, got {}", lobbies.size());

            sessionLatch.arrive_and_wait();
        });

        // attempt to login with the created accounts
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { test.network, kAddress, port };
            std::string name = newClientName(i);
            std::string password = "password";

            errors.expect(client.login(name, password), "Failed to login {}", name);

            errors.expect(client.createLobby(fmt::format("lobby{}", i)), "Failed to create lobby {}", i);

            alert.count_down();
            sessionLatch.arrive_and_wait();
        });
    }
}
