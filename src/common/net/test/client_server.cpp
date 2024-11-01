#include "net_test_common.hpp"

#include <array>
#include <thread>

#include "net/net.hpp"

using namespace sm;
using namespace sm::net;

static const char kMessage[] = "Hello, world!";
static constexpr int kClientCount = 10;

TEST_CASE("Network client server connection") {
    net::create();

    Network network = Network::create();
    ListenSocket server = network.bind(Address::loopback(), 9999);

    NetTestStream errors;
    std::atomic<int> clientCount = 0;

    std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
        server.listen(32).throwIfFailed();

        while (!stop.stop_requested()) {
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

            size_t sent = client->sendBytes(kMessage, sizeof(kMessage)).value();
            if (sent != sizeof(kMessage)) {
                errors.add("Sent {} bytes, expected {}", sent, sizeof(kMessage));
                break;
            }
        }
    });

    std::jthread clientThread = std::jthread([&] {
        for (int i = 0; i < kClientCount; ++i) {
            auto client = network.connect(Address::loopback(), 9999);

            std::array<char, 1024> buffer;
            size_t received = client.recvBytes(buffer.data(), buffer.size()).value();
            if (received != sizeof(kMessage)) {
                errors.add("Received {} bytes, expected {}", received, sizeof(kMessage));
            }

            if (std::memcmp(buffer.data(), kMessage, sizeof(kMessage)) != 0) {
                errors.add("Received unexpected data: {}", std::string_view{buffer.data(), received});
            }

            clientCount += 1;
        }
    });

    clientThread.join();

    serverThread.request_stop();
    serverThread.join();

    CHECK(clientCount == kClientCount);
}
