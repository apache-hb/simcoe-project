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
            try {
                ready.arrive_and_wait();
                fn(i, stop);
            } catch (const errors::AnyException& e) {
                for (const auto& frame : e.stacktrace()) {
                    fmt::print(stderr, "{}\n", frame.description());
                }

                throw;
            }
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
        std::latch latch{1};
        std::jthread thread = std::jthread([&, count](const std::stop_token& stop) {
            try {
                fmt::println(stderr, "Begin server");

                std::stop_callback cb(stop, [&] { 
                    fmt::println(stderr, "Cancel server");
                    server.stop(); 
                });

                latch.count_down();

                server.listen(count);
            } catch (const errors::AnyException& e) {
                fmt::println(stderr, "Server exception: {}", e.what());
                for (const auto& frame : e.stacktrace()) {
                    fmt::print(stderr, "{}\n", frame.description());
                }

                throw;
            } catch (const std::exception& e) {
                errors.add("Server exception: {}", e.what());
            }
        });

        latch.arrive_and_wait();

        return thread;
    }
};