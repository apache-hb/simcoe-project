#pragma once

#include "test/db_test_common.hpp"
#include "test/net_test_common.hpp"

#include "account/account.hpp"

#include "account.dao.hpp"

#include <latch>

using namespace sm;

namespace acd = sm::dao::account;

std::string newClientName(int i);
void createTestAccounts(net::Network& network, net::Address address, uint16_t port, NetTestStream& errors, int count);

void doParallel(int iters, auto&& fn) {
    std::vector<std::jthread> threads;
    threads.reserve(iters);
    std::latch ready{iters};
    for (int i = 0; i < iters; i++) {
        threads.emplace_back(std::jthread([&fn, &ready, i](const std::stop_token& stop) {
            ready.arrive_and_wait();
            fn(i, stop);
        }));
    }
}

struct TestServerConfig {
    net::Network network;
    db::Environment env;
    db::ConnectionConfig config;

    TestServerConfig(const std::string& path)
        : network(net::Network::create())
        , env(db::Environment::create(db::DbType::eSqlite3))
        , config(makeSqliteTestDb(path))
    {
        std::filesystem::remove(config.host);

        // drop any existing tables so we can test from a clean slate
        auto db = connect();
        db.dropTableIfExists(dao::account::Message::table());
        db.dropTableIfExists(dao::account::User::table());
    }

    db::Connection connect() {
        return env.connect(config);
    }

    game::AccountServer server(const net::Address& address, uint16_t port, unsigned seed) {
        auto db = connect();
        return game::AccountServer(std::move(db), network, address, port, seed);
    }

    std::jthread run(game::AccountServer& server, NetTestStream& errors, int count = 20) {
        auto thread = std::jthread([&, count](const std::stop_token& stop) {
            try {
                std::stop_callback cb(stop, [&] { server.stop(); });
                server.listen(count);
            } catch (const std::exception& e) {
                errors.add("Server exception: {}", e.what());
            }
        });

        while (!server.isRunning())
            std::this_thread::yield();

        return thread;
    }
};