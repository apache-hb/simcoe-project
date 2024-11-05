#pragma once

#include "db/connection.hpp"

#include "net/net.hpp"

#include "account/packets.hpp"
#include "account/router.hpp"
#include "account/salt.hpp"

#include <map>
#include <mutex>

namespace game {
    class AccountServer {
        std::mutex mDbMutex;
        sm::db::Connection mAccountDb;

        sm::net::Network& mNetwork;
        sm::net::ListenSocket mServer;

        std::mutex mSaltMutex;
        Salt mSalt;

        bool authSession(SessionId id);

        bool createAccount(game::CreateAccount info);
        SessionId login(game::Login info);
        LobbyId createLobby(game::CreateLobby info);
        bool joinLobby(game::JoinLobby info);

        uint64_t getSessionList(std::span<SessionInfo> sessions);
        uint64_t getLobbyList(std::span<LobbyInfo> lobbies);

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

        SessionId mCurrentSession = UINT64_MAX;
        LobbyId mCurrentLobby = UINT64_MAX;

        std::vector<SessionInfo> mSessions;
        std::vector<LobbyInfo> mLobbies;

    public:
        AccountClient(sm::net::Network& net, const sm::net::Address& address, uint16_t port) throws(sm::net::NetException);

        bool createAccount(std::string_view name, std::string_view password);
        bool login(std::string_view name, std::string_view password);

        bool createLobby(std::string_view name);
        bool joinLobby(LobbyId id);

        void refreshSessionList();
        void refreshLobbyList();

        std::vector<SessionInfo> getSessionInfo() { return mSessions; }
        std::vector<LobbyInfo> getLobbyInfo() { return mLobbies; }
    };
}
