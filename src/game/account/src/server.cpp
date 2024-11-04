#include "stdafx.hpp"

#include "account/account.hpp"

#include "account.dao.hpp"

using namespace std::chrono_literals;

using namespace game;
using namespace sm;

namespace acd = sm::dao::account;

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

bool AccountServer::createAccount(game::CreateAccount info) {
    std::scoped_lock guard(mSaltMutex, mDbMutex);

    auto existingUser = getUserByName(mAccountDb, info.username);
    if (existingUser.has_value()) {
        LOG_WARN(GlobalLog, "account already exists: {}", info.username.text());
        return false;
    }

    std::string salt = mSalt.getSaltString(16);
    uint64_t password = hashWithSalt(info.password, salt);

    acd::User user {
        .name = std::string{info.username},
        .password = password,
        .salt = salt
    };

    auto result = mAccountDb.tryInsertReturningPrimaryKey(user);
    if (!result.has_value()) {
        LOG_WARN(GlobalLog, "failed to create account: {}", result.error());
        return false;
    }

    LOG_INFO(GlobalLog, "created account: {}", result.value());

    return true;
}

SessionId AccountServer::login(game::Login info) {
    std::scoped_lock guard(mDbMutex, mSessionMutex);

    auto user = getUserByName(mAccountDb, info.username);
    if (!user.has_value()) {
        fmt::println(stderr, "user not found: `{}`", info.username.text());
        return SessionId::empty();
    }

    uint64_t password = hashWithSalt(info.password, user->salt);
    if (user->password != password) {
        fmt::println(stderr, "invalid password for user: `{}`", info.username.text());
        return SessionId::empty();
    }

    SessionId session = mSalt.newGuid();
    mSessions[session] = UserSession {
        .mUserId = user->id
    };

    return session;
}

void AccountServer::handleClient(const std::stop_token& stop, net::Socket socket) noexcept try {
    MessageRouter router;

    router.addRoute<Ack>(PacketType::eAck, [](const Ack& req) {
        fmt::println(stderr, "received ack: {}", req.header.id);

        return Ack { req.header.id };
    });

    router.addRoute<CreateAccount>(PacketType::eCreateAccount, [this](const CreateAccount& req) {
        if (createAccount(req)) {
            return Response { req.header.id, Status::eSuccess };
        } else {
            return Response { req.header.id, Status::eFailure };
        }
    });

    router.addRoute<Login>(PacketType::eLogin, [this](const Login& req) {
        SessionId session = login(req);
        if (session == SessionId::empty()) {
            return NewSession { req.header.id, Status::eFailure, SessionId::empty() };
        } else {
            return NewSession { req.header.id, session };
        }
    });

    while (!stop.stop_requested()) {
        if (!router.handleMessage(socket)) {
            LOG_INFO(GlobalLog, "dropping client connection");
            return;
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

AccountServer::AccountServer(db::Connection db, net::Network& net, const net::Address& address, uint16_t port) noexcept(false)
    : AccountServer(std::move(db), net, address, port, std::random_device{}())
{ }

AccountServer::AccountServer(db::Connection db, net::Network& net, const net::Address& address, uint16_t port, unsigned seed) noexcept(false)
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
