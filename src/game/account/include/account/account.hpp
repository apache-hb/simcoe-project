#pragma once

#include "db/connection.hpp"

#include "net/net.hpp"

#include "account/packets.hpp"
#include "account/salt.hpp"

#include <map>
#include <mutex>

namespace game {
    struct UserSession {
        std::string mName;
        std::map<uint16_t, std::vector<std::byte>> mPendingResponses;
        std::jthread mThread;
    };

    struct GameSession {
        std::string mPlayer0;
        std::string mPlayer1;

        // only need 9*2 bits to store the board state
        uint32_t mGameState;
    };

    class AccountServer {
        std::mutex mDbMutex;
        sm::db::Connection mAccountDb;

        sm::net::Network& mNetwork;
        sm::net::ListenSocket mServer;

        std::mutex mSaltMutex;
        Salt mSalt;

        std::mutex mSessionsMutex;
        std::map<std::string, UserSession> mSessions;

        void handleCreateAccount(sm::net::Socket& socket, game::CreateAccountRequestPacket packet);
        void handleLogin(sm::net::Socket& socket, game::LoginRequestPacket packet);
        void handleClient(const std::stop_token& stop, sm::net::Socket socket) noexcept;

    public:
        AccountServer(sm::db::Connection db, sm::net::Network& net, const sm::net::Address& address, uint16_t port) throws(sm::db::DbException);
        AccountServer(sm::db::Connection db, sm::net::Network& net, const sm::net::Address& address, uint16_t port, unsigned seed) throws(sm::db::DbException);

        void listen(uint16_t connections);
        void stop();

        bool isRunning() const;
    };

    class AccountClient {
        sm::net::Socket mSocket;

    public:
        AccountClient(sm::net::Network& net, const sm::net::Address& address, uint16_t port) throws(sm::net::NetException);

        bool createAccount(std::string_view name, std::string_view password);
        bool login(std::string_view name, std::string_view password);
    };
}
