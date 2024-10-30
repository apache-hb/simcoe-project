#include "account_test_common.hpp"

#include "account/account.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9997;
static constexpr int kClientCount = 20;

namespace acd = sm::dao::account;

TEST_CASE("Account Create & Login") {
    net::create();

    TestServerConfig test{"account/login"};

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server = test.server(kAddress, kPort, 1234);

        std::jthread serverThread = test.run(server, errors, kClientCount);

        // create clients
        createTestAccounts(test.network, kAddress, kPort, errors, kClientCount);

        // attempt to login with the created accounts
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { test.network, kAddress, kPort };
            std::string name = newClientName(i);
            std::string password = "password";

            errors.expect(client.login(name, password), "Failed to login {}", name);
        });

        // login using incorrect password
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { test.network, kAddress, kPort };
            std::string name = newClientName(i);
            std::string password = "wrong";

            errors.expect(!client.login(name, password), "Login succeeded with incorrect password");
        });

        // login with a non-existent account
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { test.network, kAddress, kPort };
            std::string name = fmt::format("nonexistent{:02}", i);
            std::string password = "password";

            errors.expect(!client.login(name, password), "Login succeeded with non-existent account");
        });
    }
}