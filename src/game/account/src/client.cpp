#include "stdafx.hpp"

#include "account/account.hpp"

using namespace std::chrono_literals;

using namespace game;
using namespace sm;

// TODO: dont hardcode timeout

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

    bool success = session.response.status == Status::eSuccess;
    if (!success)
        return false;

    mCurrentSession = session.session;
    return true;
}
