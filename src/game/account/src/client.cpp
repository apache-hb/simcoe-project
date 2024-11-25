#include "stdafx.hpp"

#include "account/account.hpp"

using namespace sm;
using namespace game;

using namespace std::chrono_literals;

AccountClient::AccountClient(sm::net::Network& net, const sm::net::Address& address, uint16_t port) noexcept(false)
    : mSocket{net.connect(address, port)}
{ }

bool AccountClient::createAccount(std::string_view name, std::string_view password) {
    if (name.size() > sizeof(CreateAccount::username) || password.size() > sizeof(CreateAccount::password))
        return false;

    mSocket.send(CreateAccount { mNextId++, kClientStream, name, password }).throwIfFailed();

    if (AnyPacket packet = getNextMessage(kClientStream)) {
        Response& response = *std::bit_cast<Response*>(packet.data());
        return response.status == Status::eSuccess;
    }

    return false;
}

bool AccountClient::login(std::string_view name, std::string_view password) {
    if (name.size() > sizeof(Login::username) || password.size() > sizeof(Login::password))
        return false;

    mSocket.send(Login { mNextId++, kClientStream, name, password }).throwIfFailed();

    if (AnyPacket packet = getNextMessage(kClientStream)) {
        NewSession& session = *std::bit_cast<NewSession*>(packet.data());
        LOG_INFO(GlobalLog, "New session established with server. Assigned id {}", session.session);

        bool success = session.response.status == Status::eSuccess;
        if (!success)
            return false;

        mCurrentSession = session.session;
        return true;
    }

    return false;
}

bool AccountClient::createLobby(std::string_view name) {
    if (name.size() > sizeof(CreateLobby::name))
        return false;

    if (!isAuthed())
        return false;

    mSocket.send(CreateLobby { mNextId++, kClientStream, mCurrentSession, name }).throwIfFailed();

    if (AnyPacket packet = getNextMessage(kClientStream)) {
        NewLobby& lobby = *std::bit_cast<NewLobby*>(packet.data());

        bool success = lobby.response.status == Status::eSuccess;
        if (!success)
            return false;

        mCurrentLobby = lobby.lobby;
        return true;
    }

    return false;
}

bool AccountClient::joinLobby(LobbyId id) {
    if (!isAuthed())
        return false;

    mSocket.send(JoinLobby { mNextId++, kClientStream, mCurrentSession, id }).throwIfFailed();

    if (AnyPacket packet = getNextMessage(kClientStream)) {
        Response& response = *std::bit_cast<Response*>(packet.data());
        bool success = response.status == Status::eSuccess;
        if (!success)
            return false;

        mCurrentLobby = id;
        return true;
    }

    return false;
}

bool AccountClient::startGame() {
    if (!isAuthed())
        return false;

    if (mCurrentLobby == UINT64_MAX)
        return false;

    mSocket.send(StartGame { mNextId++, kClientStream, mCurrentSession, mCurrentLobby }).throwIfFailed();

    if (AnyPacket packet = getNextMessage(kClientStream)) {
        Response& response = *std::bit_cast<Response*>(packet.data());
        return response.status == Status::eSuccess;
    }

    return false;
}

void AccountClient::leaveLobby() {
    if (!isAuthed())
        return;

    if (mCurrentLobby == UINT64_MAX)
        return;

    mSocket.send(LeaveLobby { mNextId++, kClientStream, mCurrentSession, mCurrentLobby }).throwIfFailed();

    mCurrentLobby = UINT64_MAX;
}

static size_t getSessionListSize(const SessionList *list) {
    size_t size = std::max<size_t>(list->response.header.size, sizeof(SessionList)) - sizeof(SessionList);
    if (size % sizeof(SessionInfo) != 0)
        return 0;

    return size / sizeof(SessionInfo);
}

bool AccountClient::refreshSessionList() {
    if (!isAuthed())
        throw std::runtime_error("Not authenticated");

    std::vector<SessionInfo> sessions;

    mSocket.send(GetSessionList { mNextId++, kClientStream, mCurrentSession }).throwIfFailed();

    if (AnyPacket data = getNextMessage(kClientStream)) {
        SessionList *list = reinterpret_cast<SessionList*>(data.data());
        size_t count = getSessionListSize(list);

        sessions.reserve(count);

        for (size_t i = 0; i < count; i++) {
            sessions.push_back(list->sessions[i]);
        }

        mSessions = std::move(sessions);
        return true;
    }

    return false;
}

static size_t getLobbyListSize(const LobbyList *list) {
    size_t size = std::max<size_t>(list->response.header.size, sizeof(LobbyList)) - sizeof(LobbyList);
    if (size % sizeof(LobbyInfo) != 0)
        return 0;

    return size / sizeof(LobbyInfo);
}

bool AccountClient::refreshLobbyList() {
    if (!isAuthed())
        throw std::runtime_error("Not authenticated");

    std::vector<LobbyInfo> lobbies;

    mSocket.send(GetLobbyList { mNextId++, kClientStream, mCurrentSession }).throwIfFailed();

    if (AnyPacket data = getNextMessage(kClientStream)) {
        LobbyList *list = reinterpret_cast<LobbyList*>(data.data());
        size_t count = getLobbyListSize(list);

        lobbies.reserve(count);

        for (size_t i = 0; i < count; i++) {
            lobbies.push_back(list->lobbies[i]);
        }

        mLobbies = std::move(lobbies);
        return true;
    }

    return false;
}

AnyPacket AccountClient::getNextMessage(uint8_t stream) {
    work();
    return mSocketMux.pop(stream);
}

void AccountClient::work() {
    mSocketMux.work(mSocket, 250ms);
}