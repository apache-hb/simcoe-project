#include "account_test_common.hpp"

#include "account/account.hpp"

#include <latch>

#include "account.dao.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr uint16_t kPort = 9998;
static constexpr int kClientCount = 20;

namespace acd = sm::dao::account;

TEST_CASE("Account Create") {
    net::create();

    db::Environment env = db::Environment::create(db::DbType::eSqlite3);
    net::Network network = net::Network::create();

    // drop any existing tables so we can test from a clean slate

    db::ConnectionConfig config = makeSqliteTestDb("account/create");
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
    }

    db::Connection db2 = env.connect(config);
    // ensure all accounts were created
    std::vector<std::string> names;
    for (const acd::User& user : db2.selectAll<acd::User>()) {
        names.push_back(user.name);
    }

    std::sort(names.begin(), names.end());
    CHECK(names.size() == kClientCount);

    for (size_t i = 0; i < names.size(); i++) {
        REQUIRE(names[i] == newClientName(i));
    }
}
