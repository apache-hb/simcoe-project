#include "account_test_common.hpp"

#include <latch>

std::string newClientName(int i) {
    return fmt::format("test{:02}", i);
}

void createTestAccounts(net::Network& network, net::Address address, uint16_t port, NetTestStream& errors, int count) {
    std::latch readyClients{count};
    doParallel(count, [&](int i, auto stop) {
        auto name = newClientName(i);
        auto password = "password";

        try {
            game::AccountClient client { network, address, port };

            readyClients.count_down();

            errors.expect(client.createAccount(name, password), "Failed to create account");
        } catch (const std::exception& e) {
            errors.add("Client {} exception: {}", name, e.what());
        }
    });
}
