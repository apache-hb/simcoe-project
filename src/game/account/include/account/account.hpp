#pragma once

#include "db/connection.hpp"

#include "net/net.hpp"

#include "account/packets.hpp"
#include "account/router.hpp"
#include "account/salt.hpp"

#include <map>
#include <mutex>

namespace game {
    static constexpr uint8_t kClientStream = 0;
    static constexpr uint8_t kEventStream = 1;

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

        void dropSession(SessionId session);

    public:
        AccountServer(sm::db::Connection db, sm::net::Network& net, const sm::net::Address& address, uint16_t port) throws(sm::db::DbException);
        AccountServer(sm::db::Connection db, sm::net::Network& net, const sm::net::Address& address, uint16_t port, unsigned seed) throws(sm::db::DbException);

        SM_NOCOPY(AccountServer);
        SM_NOMOVE(AccountServer);

        void listen(uint16_t connections);
        void stop();

        bool isRunning() const;

        uint16_t getPort() { return mServer.getBoundPort(); }
    };

    class AccountClient {
        sm::net::Socket mSocket;
        SocketMux mSocketMux;
        uint16_t mNextId = 0;

        SessionId mCurrentSession = UINT64_MAX;
        LobbyId mCurrentLobby = UINT64_MAX;

        using RequestCallback = std::function<void(std::span<const std::byte>)>;

        std::map<uint16_t, RequestCallback> mRequestSlots;

        std::vector<SessionInfo> mSessions;
        std::vector<LobbyInfo> mLobbies;

    public:
        AccountClient(sm::net::Network& net, const sm::net::Address& address, uint16_t port) throws(sm::net::NetException);

        SM_NOCOPY(AccountClient);
        SM_NOMOVE(AccountClient);

        bool isAuthed() { return mCurrentSession != UINT64_MAX; }

        bool createAccount(std::string_view name, std::string_view password);
        bool login(std::string_view name, std::string_view password);

        bool createLobby(std::string_view name);
        bool joinLobby(LobbyId id);
        bool startGame();
        void leaveLobby();

        bool sendMessage(std::string_view message);

        bool refreshSessionList();
        bool refreshLobbyList();

        AnyPacket getNextMessage(uint8_t stream);

        void work();

        std::vector<SessionInfo> getSessionInfo() { return mSessions; }
        std::vector<LobbyInfo> getLobbyInfo() { return mLobbies; }
    };
}
