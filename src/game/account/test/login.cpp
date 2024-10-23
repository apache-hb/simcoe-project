#include "account_test_common.hpp"

#include "account/account.hpp"

#include "account.dao.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9997;
static constexpr int kClientCount = 20;

namespace acd = sm::dao::account;

TEST_CASE("Account Create & Login") {
    net::create();

    db::Environment env = db::Environment::create(db::DbType::eSqlite3);
    net::Network network = net::Network::create();

    // drop any existing tables so we can test from a clean slate

    db::ConnectionConfig config = makeSqliteTestDb("account/login");
    db::Connection db = env.connect(config);
    db.dropTableIfExists(acd::Message::table());
    db.dropTableIfExists(acd::User::table());

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server { std::move(db), network, kAddress, kPort, 1234 };

        std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
            try {
                std::stop_callback cb(stop, [&] { server.stop(); });
                server.listen(kClientCount);
            } catch (const std::exception& e) {
                errors.add("Server exception: {}", e.what());
            }
        });

        // create clients
        createTestAccounts(network, kAddress, kPort, errors, kClientCount);

        // attempt to login with the created accounts
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { network, kAddress, kPort };
            std::string name = newClientName(i);
            std::string password = "password";

            errors.expect(client.login(name, password), "Failed to login {}", name);
        });

        // login using incorrect password
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { network, kAddress, kPort };
            std::string name = newClientName(i);
            std::string password = "wrong";

            errors.expect(!client.login(name, password), "Login succeeded with incorrect password");
        });

        // login with a non-existent account
        doParallel(kClientCount, [&](int i, auto stop) {
            game::AccountClient client { network, kAddress, kPort };
            std::string name = fmt::format("nonexistent{:02}", i);
            std::string password = "password";

            errors.expect(!client.login(name, password), "Login succeeded with non-existent account");
        });
    }
}
