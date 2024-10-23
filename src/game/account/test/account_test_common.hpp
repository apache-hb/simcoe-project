#pragma once

#include "test/db_test_common.hpp"
#include "test/net_test_common.hpp"

#include "account/account.hpp"

using namespace sm;

std::string newClientName(int i);
void createTestAccounts(net::Network& network, net::Address address, uint16_t port, NetTestStream& errors, int count);

void doParallel(int iters, auto&& fn) {
    std::vector<std::jthread> threads;
    threads.reserve(iters);
    for (int i = 0; i < iters; i++) {
        threads.emplace_back(std::jthread([&, i](const std::stop_token& stop) {
            fn(i, stop);
        }));
    }
}
