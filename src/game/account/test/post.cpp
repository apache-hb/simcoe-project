#include "account_test_common.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr int kClientCount = 20;
// static constexpr int kMessageCount = 3;

namespace acd = sm::dao::account;

std::string makeMessage(int i, std::string_view name) {
    return fmt::format("Hello, world {}! from {}", i, name);
}

TEST_CASE("Account post message") {
    SKIP("Not implemented");

    net::create();

    TestServerConfig test{"account/post"};

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server = test.server(kAddress, 0, 1234);
        uint16_t port = server.getPort();

        auto serverThread = test.run(server, errors, kClientCount);

        // create clients
        createTestAccounts(test.network, kAddress, port, errors, kClientCount);
    }

    db::Connection db2 = test.connect();
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
