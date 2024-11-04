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
    CreateAccountRequestPacket packet{};

    if (name.size() > sizeof(packet.username) || password.size() > sizeof(packet.password))
        return false;

    std::memcpy(packet.username, name.data(), name.size());
    std::memcpy(packet.password, password.data(), password.size());

    mSocket.send(packet).throwIfFailed();

    auto response = net::throwIfFailed(mSocket.recvTimed<CreateAccountResponsePacket>(1s));

    return response.status == Status::eSuccess;
}

bool AccountClient::login(std::string_view name, std::string_view password) {
    LoginRequestPacket packet{};

    if (name.size() > sizeof(packet.username) || password.size() > sizeof(packet.password))
        return false;

    std::memcpy(packet.username, name.data(), name.size());
    std::memcpy(packet.password, password.data(), password.size());

    mSocket.send(packet).throwIfFailed();

    LoginResponsePacket response = net::throwIfFailed(mSocket.recvTimed<LoginResponsePacket>(1s));

    bool success = response.status == Status::eSuccess;

    return success;
}
