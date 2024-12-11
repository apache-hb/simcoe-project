#include "account_test_common.hpp"
#include "account/account.hpp"

using namespace sm;

static constexpr net::Address kAddress = net::Address::loopback();

namespace acd = sm::dao::account;

TEST_CASE("Join lobby and send messages") {
    if (!net::isSetup())
        net::create();

    TestServerConfig test{"account/send_messages"};

    {
        NetTestStream errors;

        // setup account server
        game::AccountServer server = test.server(kAddress, 0, 1234);
        uint16_t port = server.getPort();

        auto serverThread = test.run(server, errors, kClientCount);

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

        CHECK(client0.sendMessage("Hello, world!"));

        CHECK(client1.sendMessage("Hello, back!"));

        // usually these are run on worker threads, but
        // for testing its easier to run on the main thread
        client0.work();
        client1.work();
        client0.refreshMessageList();
        client1.refreshMessageList();

        REQUIRE(client0.getMessages().size() == 2);
        REQUIRE(client1.getMessages().size() == 2);
        CHECK(client0.getMessages()[1].message == "Hello, back!");
        CHECK(client1.getMessages()[1].message == "Hello, world!");
    }
}
