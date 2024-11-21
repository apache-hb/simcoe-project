#include <catch2/benchmark/catch_benchmark.hpp>
#include <mutex>

#include "test/db_test_common.hpp"

#include "threads/mailbox.hpp"

using namespace sm;
using namespace std::chrono_literals;

static constexpr size_t kCacheLineSize = std::hardware_constructive_interference_size;

template<size_t N>
static void verifyArray(const std::array<uint8_t, N>& data) {
    uint8_t first = data[0];
    uint8_t last = data[N - 1];
    for (size_t i = 0; i < N; i += kCacheLineSize) {
        uint8_t value = data[i];
        if (value != first || value != last) {
            FAIL(fmt::format("Mismatch at index {} expected {} got {}", i, first, value));
        }
    }
}

static constexpr size_t kBigArraySize = 0x10000;
static constexpr size_t kMediumArraySize = 0x1000;
static constexpr size_t kSmallArraySize = 0x100;

using BigArray = std::array<uint8_t, kBigArraySize>;
using MediumArray = std::array<uint8_t, kMediumArraySize>;
using SmallArray = std::array<uint8_t, kSmallArraySize>;

std::unique_ptr bigMailbox = std::make_unique<threads::NonBlockingMailBox<BigArray>>();
std::unique_ptr mediumMailbox = std::make_unique<threads::NonBlockingMailBox<MediumArray>>();
std::unique_ptr smallMailbox = std::make_unique<threads::NonBlockingMailBox<SmallArray>>();

TEST_CASE("Mailbox baseline") {
    BENCHMARK("Baseline reading mailbox of big array") {
        std::lock_guard guard(*bigMailbox);
        const auto& data = bigMailbox->read();
        verifyArray(data);
    };

    BENCHMARK("Baseline reading mailbox of medium array") {
        std::lock_guard guard(*mediumMailbox);
        const auto& data = mediumMailbox->read();
        verifyArray(data);
    };

    BENCHMARK("Baseline reading mailbox of small array") {
        std::lock_guard guard(*smallMailbox);
        const auto& data = smallMailbox->read();
        verifyArray(data);
    };
}

TEST_CASE("Mailbox writer overhead") {
    std::jthread writer = std::jthread([&](const std::stop_token& stop) {
        std::array<uint8_t, kBigArraySize> bigData;
        std::array<uint8_t, kMediumArraySize> mediumData;
        std::array<uint8_t, kSmallArraySize> smallData;

        uint8_t value = 0;
        while (!stop.stop_requested()) {
            uint8_t fill = value++;

            bigData.fill(fill);
            mediumData.fill(fill);
            smallData.fill(fill);

            bigMailbox->write(bigData);
            mediumMailbox->write(mediumData);
            smallMailbox->write(smallData);
        }
    });

    BENCHMARK("Reading big array from mailbox") {
        std::lock_guard guard(*bigMailbox);
        const auto& data = bigMailbox->read();
        verifyArray(data);
    };

    BENCHMARK("Reading medium array from mailbox") {
        std::lock_guard guard(*mediumMailbox);
        const auto& data = mediumMailbox->read();
        verifyArray(data);
    };

    BENCHMARK("Reading small array from mailbox") {
        std::lock_guard guard(*smallMailbox);
        const auto& data = smallMailbox->read();
        verifyArray(data);
    };
}
