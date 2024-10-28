#include "stdafx.hpp"

#include "account/account.hpp"

#include "account.dao.hpp"

#include <unordered_set>

using namespace std::chrono_literals;

using namespace game;
using namespace sm;

namespace acd = sm::dao::account;

std::string AccountServer::newSaltString(size_t length) {
    std::lock_guard lock(mSaltMutex);
    return mSalt.getSaltString(length);
}

static bool recvDataPacket(std::span<std::byte> dst, net::Socket& socket, game::PacketHeader header) {
    std::memcpy(dst.data(), &header, sizeof(header));
    size_t size = game::getPacketDataSize(header);

    size_t read = socket.recvBytes(dst.data() + sizeof(header), size).value();
    if (read != size) {
        LOG_ERROR(GlobalLog, "failed to receive all packet data: expected {}, got {}", size, read);
        return false;
    }

    return true;
}

static std::optional<acd::User> getUserByName(db::Connection& db, std::string_view name) try {
    // TODO: horrible evil, need to support arbitrary prepared statements
    std::string query = fmt::format("SELECT * FROM user WHERE name = '{}'", name);

    return db.selectOneWhere<acd::User>(query);
} catch (const db::DbException& e) {
    if (e.error().noData()) {
        return std::nullopt;
    }

    throw;
}

void AccountServer::handleCreateAccount(sm::net::Socket& socket, game::CreateAccountRequestPacket packet) {
    auto sendResponse = [&](CreateAccountStatus status) {
        CreateAccountResponsePacket response{};
        response.status = status;

        socket.send(response).throwIfFailed();
    };

    std::string salt = newSaltString(16);
    uint64_t password = hashWithSalt(packet.password, salt);

    acd::User user {
        .name = packet.username,
        .password = password,
        .salt = salt
    };

    std::lock_guard guard(mDbMutex);

    auto oldUser = getUserByName(mAccountDb, packet.username);
    if (oldUser.has_value()) {
        LOG_WARN(GlobalLog, "account already exists: {}", user.name);
        sendResponse(CreateAccountStatus::eFailure);
        return;
    }

    auto result = mAccountDb.tryInsertReturningPrimaryKey(user);

    if (!result.has_value()) {
        LOG_WARN(GlobalLog, "failed to create account: {}", result.error());
        sendResponse(CreateAccountStatus::eFailure);
        return;
    }

    LOG_INFO(GlobalLog, "created account: {}", result.value());
    sendResponse(CreateAccountStatus::eSuccess);
}

void AccountServer::handleLogin(sm::net::Socket& socket, game::LoginRequestPacket packet) try {
    auto sendResponse = [&](LoginResult result) {
        LoginResponsePacket response;
        response.result = result;

        socket.send(response).throwIfFailed();
    };

    std::string name = packet.username;

    std::lock_guard guard(mDbMutex);
    std::optional<acd::User> optUser = getUserByName(mAccountDb, name);
    if (!optUser.has_value()) {
        sendResponse(LoginResult::eFailure);
        return;
    }

    const acd::User& user = optUser.value();

    uint64_t password = hashWithSalt(packet.password, user.salt);

    if (user.password == password) {
        sendResponse(LoginResult::eSuccess);
    } else {
        sendResponse(LoginResult::eFailure);
    }
} catch (const db::DbException& e) {
    LoginResponsePacket response;
    response.result = LoginResult::eFailure;

    socket.send(response).throwIfFailed();
}

void AccountServer::handleClient(const std::stop_token& stop, net::Socket socket) noexcept try {
    while (!stop.stop_requested()) {
        auto headerOr = socket.recv<game::PacketHeader>();
        if (net::isConnectionClosed(headerOr)) {
            LOG_INFO(GlobalLog, "client disconnected");
            break;
        }

        auto header = net::throwIfFailed(std::move(headerOr));

        std::byte buffer[512];
        if (!recvDataPacket(buffer, socket, header))
            break;

        switch (header.type) {
        case game::PacketType::eCreateAccountRequest:
            handleCreateAccount(socket, *reinterpret_cast<game::CreateAccountRequestPacket*>(buffer));
            break;

        case game::PacketType::eLoginRequest:
            handleLogin(socket, *reinterpret_cast<game::LoginRequestPacket*>(buffer));
            break;

        default:
            LOG_WARN(GlobalLog, "unknown packet type: {}", header.type);
            break;
        }
    }
} catch (const db::DbException& e) {
    LOG_WARN(GlobalLog, "database error: {}", e.error());
} catch (const net::NetException& e) {
    LOG_WARN(GlobalLog, "network error: {}", e);
} catch (const std::exception& e) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", e.what());
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
}

static void createSchema(db::Connection& db) {
    db.createTable(acd::User::table());
    db.createTable(acd::Message::table());
}

AccountServer::AccountServer(db::Connection db, net::Network& net, net::Address address, uint16_t port) noexcept(false)
    : mAccountDb(std::move(db))
    , mNetwork(net)
    , mServer(mNetwork.bind(address, port))
{
    createSchema(mAccountDb);
}

AccountServer::AccountServer(db::Connection db, net::Network& net, net::Address address, uint16_t port, unsigned seed) noexcept(false)
    : mAccountDb(std::move(db))
    , mNetwork(net)
    , mServer(mNetwork.bind(address, port))
    , mSalt(seed)
{
    createSchema(mAccountDb);
}

void AccountServer::listen(uint16_t connections) {
    mServer.listen(connections).throwIfFailed();

    std::unordered_set<std::unique_ptr<std::jthread>> threads;
    while (mServer.isActive()) {
        net::NetResult<net::Socket> client = mServer.tryAccept();
        if (isCancelled(client))
            break;

        net::Socket socket = net::throwIfFailed(std::move(client));

        threads.emplace(std::make_unique<std::jthread>([this, socket = std::move(socket)](const std::stop_token& stop) mutable {
            handleClient(stop, std::move(socket));
        }));
    }
}

void AccountServer::stop() {
    mServer.cancel();
}

bool AccountServer::isRunning() const {
    return mServer.isActive();
}
