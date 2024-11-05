#include "account_test_common.hpp"

#include "account/account.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9981;
static constexpr int kClientCount = 20;

namespace acd = sm::dao::account;

TEST_CASE("Account Create & Login") {
    net::create();

    TestServerConfig test{"account/session_list"};

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server = test.server(kAddress, kPort, 1234);

        std::jthread serverThread = test.run(server, errors, kClientCount);

        // create clients
        createTestAccounts(test.network, kAddress, kPort, errors, kClientCount);


        // login with the created accounts but dont close connections
        std::latch sessionLatch{kClientCount + 1};
        std::latch alert{kClientCount + 1};

        std::jthread queryThread = std::jthread([&](const std::stop_token& stop) {
            game::AccountClient client { test.network, kAddress, kPort };

            client.createAccount("scanner", "password");
            client.login("scanner", "password");

            alert.arrive_and_wait();

            client.refreshSessionList();

            auto info = client.getSessionInfo();

            errors.expect(info.size() >= kClientCount, "Expected at least {} sessions, got {}", kClientCount, info.size());

            sessionLatch.arrive_and_wait();
        });

        // attempt to login with the created accounts
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { test.network, kAddress, kPort };
            std::string name = newClientName(i);
            std::string password = "password";

            errors.expect(client.login(name, password), "Failed to login {}", name);

            alert.count_down();
            sessionLatch.arrive_and_wait();
        });
    }
}
