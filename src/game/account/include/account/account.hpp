#pragma once

#include "db/connection.hpp"

#include "net/net.hpp"

#include "account/packets.hpp"
#include "account/router.hpp"
#include "account/salt.hpp"

#include <map>
#include <mutex>

namespace game {
    struct UserSession {
        uint64_t mUserId;
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

        std::mutex mSessionMutex;
        std::unordered_map<SessionId, UserSession> mSessions;

        bool createAccount(game::CreateAccount info);
        SessionId login(game::Login info);

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
        uint16_t mNextId = 0;

        SessionId mCurrentSession;

    public:
        AccountClient(sm::net::Network& net, const sm::net::Address& address, uint16_t port) throws(sm::net::NetException);

        bool createAccount(std::string_view name, std::string_view password);
        bool login(std::string_view name, std::string_view password);
    };
}
