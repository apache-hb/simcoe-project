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
            fmt::println(stderr, "Before prepare {}", name);

            game::AccountClient client { network, address, port };

            fmt::println(stderr, "Preparing for {}", name);

            readyClients.arrive_and_wait();

            fmt::println(stderr, "Creating {}", name);

            errors.expect(client.createAccount(name, password), "Failed to create account");
        } catch (const std::exception& e) {
            fmt::println(stderr, "oh no");
            errors.add("Client {} exception: {}", name, e.what());
        } catch (...) {
            fmt::println(stderr, "uhhh");
        }

        fmt::println(stderr, "done");
    });
}
