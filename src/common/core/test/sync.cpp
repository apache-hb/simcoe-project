#include <gtest/gtest.h>

#include "core/sync.hpp"

#include <algorithm>
#include <vector>
#include <thread>

#include <fmtlib/format.h>

using ExclusiveVector = sm::ExclusiveSync<std::vector<int>>;
using SharedVector = sm::Sync<std::vector<int>>;

TEST(SyncTest, ExclusiveSync) {
    ExclusiveVector vec;

    const int count = 32;
    {
        std::vector<std::jthread> threads;
        threads.reserve(count);
        for (int i = 0; i < count; i++) {
            threads.emplace_back([i, &vec] {
                auto guard = vec.take();
                guard->push_back(i * 100);
            });
        }
    }

    auto guard = vec.take();

    std::sort(guard->begin(), guard->end());
    for (int i = 0; i < count; i++) {
        ASSERT_EQ(guard->at(i), i * 100);
    }
}

TEST(SyncTest, Sync) {
    SharedVector vec;
    sm::ExclusiveSync<std::vector<std::string>> errors;
    const int count = 32;
    {
        // TODO: this test isnt very good, but i think it proves that there
        // are no race conditions in the Sync class
        std::vector<std::jthread> threads;
        threads.reserve(count);
        for (int i = 0; i < count; i++) {
            threads.emplace_back([&, i] {
                if (i % 2 == 0) {
                    auto guard = vec.acquire();
                    guard->push_back(i * 100);
                } else {
                    auto guard = vec.read();
                    size_t size = guard->size();
                    std::vector<int> copy = *guard;
                    std::sort(copy.begin(), copy.end());
                    if (size != copy.size()) {
                        auto err = errors.take();
                        err->push_back(fmt::format("Size mismatch at index {}, expected {}, found {}", i, size, copy.size()));
                    }

                    if (size > count) {
                        auto err = errors.take();
                        err->push_back(fmt::format("Size too large at index {}, found {}", i, size));
                        return;
                    }

                    for (size_t j = 0; j < size; j++) {
                        if (copy[j] % 200 != 0) {
                            auto err = errors.take();
                            err->push_back(fmt::format("Value mismatch at index {}, expected {}, found {}", i, j * 200, copy[j]));
                        }
                    }
                }
            });
        }
    }

    auto guard = vec.acquire();
    std::sort(guard->begin(), guard->end());

    for (int i = 0; i < count / 2; i++) {
        ASSERT_EQ(guard->at(i), i * 200);
    }

    auto err = errors.take();

    EXPECT_TRUE(err->empty());
    for (const auto& e : *err) {
        fmt::println(stderr, "{}", e);
    }
}