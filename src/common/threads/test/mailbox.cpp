#define _GLIBCXX_DEBUG

#include "test/common.hpp"

#include "threads/mailbox.hpp"

#include <fmtlib/format.h>

#include <array>
#include <latch>
#include <mutex>
#include <thread>

using namespace sm;

using namespace std::chrono_literals;

struct MultiThreadedTestStream {
    std::mutex errorMutex;
    std::vector<std::string> errors;

    void add(std::string_view fmt, auto&&... args) {
        std::lock_guard guard(errorMutex);
        errors.push_back(fmt::vformat(fmt, fmt::make_format_args(args...)));
    }

    bool expect(bool condition, std::string_view fmt, auto&&... args) {
        if (!condition)
            add(fmt, std::forward<decltype(args)>(args)...);

        return condition;
    }

    ~MultiThreadedTestStream() noexcept(false) {
        CHECK(errors.empty());
        int limit = 100;
        for (const auto& error : errors) {
            FAIL_CHECK(error);

            if (--limit <= 0) {
                break;
            }
        }
    }
};

TEST_CASE("Non-blocking mailbox") {
    static constexpr size_t kArraySize = 0x10000;
    using BigArray = std::array<uint8_t, kArraySize>;
    using BigArrayMailbox = threads::NonBlockingMailBox<BigArray>;

    // a little too big for the stack
    auto ptr = std::make_unique<BigArrayMailbox>();

    {
        MultiThreadedTestStream errors;

        std::latch latch{1};

        std::jthread reader = std::jthread([ptr = ptr.get(), &latch, &errors](const std::stop_token& stop) {
            latch.wait();

            while (!stop.stop_requested()) {
                std::lock_guard guard(*ptr);

                const BigArray& data = ptr->read();
                uint8_t first = data[0];
                uint8_t last = data[kArraySize - 1];

                errors.expect(first != 0 && last != 0, "first = {}, last = {}", first, last);
                errors.expect(first == last, "first = {}, last = {}", first, last);
            }
        });

        std::jthread writer = std::jthread([ptr = ptr.get(), &latch](const std::stop_token& stop) {
            uint8_t value = 1;
            std::once_flag once;
            while (!stop.stop_requested()) {
                BigArray data;
                uint8_t next = value++;
                if (next == 0) {
                    value = 1;
                    next = 1;
                }

                data.fill(next);

                ptr->write(data);

                std::call_once(once, [&] {  latch.count_down(); });
            }
        });

        std::this_thread::sleep_for(3s);
    }

    SUCCEED("No crashes or torn reads");
}
