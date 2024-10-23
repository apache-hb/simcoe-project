#pragma once

#include "db/connection.hpp"

#include "net/net.hpp"

#include "account/packets.hpp"
#include "account/salt.hpp"

#include <mutex>

namespace game {
    class AccountServer {
        std::mutex mDbMutex;
        sm::db::Connection mAccountDb;

        sm::net::Network& mNetwork;
        sm::net::ListenSocket mServer;

        std::mutex mSaltMutex;
        Salt mSalt;

        std::string newSaltString(size_t length);

        void handleCreateAccount(sm::net::Socket& socket, game::CreateAccountRequestPacket packet);
        void handleLogin(sm::net::Socket& socket, game::LoginRequestPacket packet);

        void handleClient(const std::stop_token& stop, sm::net::Socket socket) noexcept;

    public:
        AccountServer(sm::db::Connection db, sm::net::Network& net, sm::net::Address address, uint16_t port) throws(sm::db::DbException);
        AccountServer(sm::db::Connection db, sm::net::Network& net, sm::net::Address address, uint16_t port, unsigned seed) throws(sm::db::DbException);

        void listen(uint16_t connections);
        void stop();
    };

    class AccountClient {
        sm::net::Socket mSocket;

    public:
        AccountClient(sm::net::Network& net, sm::net::Address address, uint16_t port) throws(sm::net::NetException);

        bool createAccount(std::string_view name, std::string_view password);
        bool login(std::string_view name, std::string_view password);
    };
}
