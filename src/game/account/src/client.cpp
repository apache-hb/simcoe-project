#include "stdafx.hpp"

#include "account/account.hpp"

using namespace sm;
using namespace game;

using namespace std::chrono_literals;

static std::unique_ptr<std::byte[]> readFlexiblePacket(sm::net::Socket& socket) {
    PacketHeader header = net::throwIfFailed(socket.recv<PacketHeader>());
    std::unique_ptr<std::byte[]> data = std::make_unique<std::byte[]>(header.size);
    memcpy(data.get(), &header, sizeof(PacketHeader));

    if (header.size > sizeof(PacketHeader)) {
        net::throwIfFailed(socket.recvBytes(data.get() + sizeof(PacketHeader), header.size - sizeof(PacketHeader)));
    }

    return data;
}

AccountClient::AccountClient(sm::net::Network& net, const sm::net::Address& address, uint16_t port) noexcept(false)
    : mSocket{net.connect(address, port)}
{ }

bool AccountClient::createAccount(std::string_view name, std::string_view password) {
    if (name.size() > sizeof(CreateAccount::username) || password.size() > sizeof(CreateAccount::password))
        return false;

    mSocket.send(CreateAccount { mNextId++, name, password }).throwIfFailed();

    Response response = net::throwIfFailed(mSocket.recv<Response>());

    return response.status == Status::eSuccess;
}

bool AccountClient::login(std::string_view name, std::string_view password) {
    if (name.size() > sizeof(Login::username) || password.size() > sizeof(Login::password))
        return false;

    mSocket.send(Login { mNextId++, name, password }).throwIfFailed();

    NewSession session = net::throwIfFailed(mSocket.recv<NewSession>());
    LOG_INFO(GlobalLog, "New session established with server. Assigned id {}", session.session);

    bool success = session.response.status == Status::eSuccess;
    if (!success)
        return false;

    mCurrentSession = session.session;
    return true;
}

bool AccountClient::createLobby(std::string_view name) {
    if (name.size() > sizeof(CreateLobby::name))
        return false;

    mSocket.send(CreateLobby { mNextId++, mCurrentSession, name }).throwIfFailed();

    NewLobby lobby = net::throwIfFailed(mSocket.recv<NewLobby>());

    bool success = lobby.response.status == Status::eSuccess;
    if (!success)
        return false;

    mCurrentLobby = lobby.lobby;
    return true;
}

bool AccountClient::joinLobby(LobbyId id) {
    mSocket.send(JoinLobby { mNextId++, mCurrentSession, id }).throwIfFailed();

    Response response = net::throwIfFailed(mSocket.recv<Response>());

    bool success = response.status == Status::eSuccess;
    if (!success)
        return false;

    mCurrentLobby = id;
    return true;
}

static size_t getSessionListSize(const SessionList *list) {
    size_t size = std::max<size_t>(list->response.header.size, sizeof(SessionList)) - sizeof(SessionList);
    if (size % sizeof(SessionInfo) != 0)
        return 0;

    return size / sizeof(SessionInfo);
}

void AccountClient::refreshSessionList() {
    std::vector<SessionInfo> sessions;

    mSocket.send(GetSessionList { mNextId++, mCurrentSession }).throwIfFailed();

    std::unique_ptr<std::byte[]> data = readFlexiblePacket(mSocket);
    SessionList *list = reinterpret_cast<SessionList*>(data.get());
    size_t count = getSessionListSize(list);

    sessions.reserve(count);

    for (size_t i = 0; i < count; i++) {
        sessions.push_back(list->sessions[i]);
    }

    mSessions = std::move(sessions);
}

static size_t getLobbyListSize(const LobbyList *list) {
    size_t size = std::max<size_t>(list->response.header.size, sizeof(LobbyList)) - sizeof(LobbyList);
    if (size % sizeof(LobbyInfo) != 0)
        return 0;

    return size / sizeof(LobbyInfo);
}

void AccountClient::refreshLobbyList() {
    std::vector<LobbyInfo> lobbies;

    mSocket.send(GetLobbyList { mNextId++, mCurrentSession }).throwIfFailed();

    std::unique_ptr<std::byte[]> data = readFlexiblePacket(mSocket);
    LobbyList *list = reinterpret_cast<LobbyList*>(data.get());
    size_t count = getLobbyListSize(list);

    lobbies.reserve(count);

    for (size_t i = 0; i < count; i++) {
        lobbies.push_back(list->lobbies[i]);
    }

    mLobbies = std::move(lobbies);
}

std::unique_ptr<std::byte[]> AccountClient::getNextMessage(std::chrono::milliseconds timeout) {
    while (true) {
        PacketHeader header = net::throwIfFailed(mSocket.recvTimed<PacketHeader>(timeout));
        size_t size = std::max<size_t>(header.size, sizeof(PacketHeader));
        std::unique_ptr<std::byte[]> packet = std::make_unique<std::byte[]>(size);
        memcpy(packet.get(), &header, sizeof(PacketHeader));
        mSocket.recvBytesTimeout(packet.get() + sizeof(PacketHeader), size - sizeof(PacketHeader), timeout).error.throwIfFailed();

        if (auto it = mRequestSlots.find(header.id); it != mRequestSlots.end()) {
            std::invoke(it->second, std::span(packet.get(), size));
            mRequestSlots.erase(it);
            continue;
        } 
        
        return packet;
    }
}
