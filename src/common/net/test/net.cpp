#include "test/common.hpp"

#include <array>
#include <mutex>
#include <thread>

#include "net/net.hpp"

using namespace sm::net;

static const char kMessage[] = "Hello, world!";
static constexpr int kClientCount = 10;

TEST_CASE("Network client server connection") {
    auto network = Network::create();
    auto server = network.bind(IPv4Address::loopback(), 9999);

    std::mutex errorMutex;
    std::vector<std::string> errors;
    std::atomic<int> clientCount = 0;

    auto addError = [&](std::string_view fmt, auto&&... args) {
        std::lock_guard guard(errorMutex);
        errors.push_back(fmt::vformat(fmt, fmt::make_format_args(args...)));
    };

    std::jthread serverThread = std::jthread([&](const std::stop_token& stop) {
        server.listen(network.getMaxSockets()).throwIfFailed();

        while (!stop.stop_requested()) {
            auto client = [&] {
                std::stop_callback cb(stop, [&] { server.cancel(); });
                return server.tryAccept();
            }();

            if (!client.has_value()) {
                const auto& err = client.error();
                if (err.cancelled())
                    break;
            }

            size_t sent = client->sendBytes(kMessage, sizeof(kMessage)).value();
            if (sent != sizeof(kMessage)) {
                addError("Sent {} bytes, expected {}", sent, sizeof(kMessage));
                break;
            }
        }
    });

    std::jthread clientThread = std::jthread([&] {
        for (int i = 0; i < kClientCount; ++i) {
            auto client = network.connect(IPv4Address::loopback(), 9999);

            std::array<char, 1024> buffer;
            size_t received = client.recvBytes(buffer.data(), buffer.size()).value();
            if (received != sizeof(kMessage)) {
                addError("Received {} bytes, expected {}", received, sizeof(kMessage));
            }

            if (std::memcmp(buffer.data(), kMessage, sizeof(kMessage)) != 0) {
                addError("Received unexpected data: {}", std::string_view{buffer.data(), received});
            }

            clientCount += 1;
        }
    });

    clientThread.join();

    serverThread.request_stop();
    serverThread.join();

    CHECK(errors.empty());
    CHECK(clientCount == kClientCount);

    for (const auto& error : errors) {
        FAIL(error);
    }
}
