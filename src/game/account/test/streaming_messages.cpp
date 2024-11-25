#include "account_test_common.hpp"
#include "account/account.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();
static constexpr int kClientCount = 20;

namespace acd = sm::dao::account;

TEST_CASE("Join lobby and send messages") {
    if (!net::isSetup())
        net::create();

    TestServerConfig test{"account/join_lobby"};

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server = test.server(kAddress, 0, 1234);
        uint16_t port = server.getPort();

        std::jthread serverThread = test.run(server, errors, kClientCount);

        game::AccountClient client0 { test.network, kAddress, port };
        game::AccountClient client1 { test.network, kAddress, port };

        errors.expect(client0.createAccount("client0", "password"), "failed to create client0");
        errors.expect(client0.login("client0", "password"), "failed to login to client0");

        errors.expect(client1.createAccount("client1", "password"), "failed to create client1");
        errors.expect(client1.login("client1", "password"), "failed to login to client1");

        errors.expect(client0.createLobby("lobby0"), "failed to create lobby0");

        errors.expect(client1.refreshLobbyList(), "failed to refresh lobby list");

        errors.expect(client1.getLobbyInfo().size() == 1, "Expected 1 lobby, got {}", client1.getLobbyInfo().size());

        client1.joinLobby(client1.getLobbyInfo()[0].id);

        client0.sendMessage("Hello, world!");

        client1.sendMessage("Hello, back!");
    }
}
