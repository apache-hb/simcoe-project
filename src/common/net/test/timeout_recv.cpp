#include "net_test_common.hpp"

#include <thread>

#include "net/net.hpp"

using namespace sm::net;

using namespace std::chrono_literals;

static constexpr size_t kBufferSize = 128;

TEST_CASE("Network client server connection") {
    auto network = Network::create();

    NetTestStream errors;

    auto server = network.bind(IPv4Address::loopback(), 9999);

    REQUIRE(server.setBlocking(false).isSuccess());
    REQUIRE(server.setBlocking(true).isSuccess());
    REQUIRE(server.listen(1).isSuccess());

    std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
        while (stop.stop_requested()) {
            auto client = [&] {
                std::stop_callback cb(stop, [&] { server.cancel(); });
                return server.tryAccept();
            }();

            if (!client.has_value()) {
                const auto& err = client.error();
                if (!err.cancelled())
                    errors.add("Failed to accept client: {}", err.message());

                break;
            }

            char buffer[kBufferSize];
            for (size_t i = 0; i < sizeof(buffer); ++i) {
                buffer[i] = std::hash<size_t>{}(i) % CHAR_MAX;
            }

            size_t sent = client->sendBytes(buffer, sizeof(buffer)).value();
            errors.expect(sent == sizeof(buffer), "Sent {} bytes, expected {}", sent, sizeof(buffer));
        }
    });

    auto client = network.connect(IPv4Address::loopback(), 9999);
    client.setBlocking(false).throwIfFailed();

    // recv a buffer of 256, only 128 will ever be sent. this should timeout
    char buffer[kBufferSize * 2];
    auto [read, err] = client.recvBytesTimeout(buffer, sizeof(buffer), 256ms);
    errors.expect(err.timeout(), "Expected timeout, got: {}", err.message());
}
