#include "test/common.hpp"

#include "core/adt/small_vector.hpp"

TEST_CASE("vector operations") {
    GIVEN("a mutable vector") {
        sm::SmallVector<std::string, 64> vec;

        THEN("it is empty by default") {
            CHECK(vec.isEmpty());
            CHECK(vec.size() == 0);
            CHECK(vec.sizeInBytes() == 0);
        }

        THEN("copying the vector copies its elements") {
            vec.emplace_back("hello");
            vec.emplace_back("world");

            sm::SmallVector<std::string, 64> copy = sm::SmallVector{vec};

            CHECK(copy.size() == 2);
            CHECK(copy[0] == "hello");
            CHECK(copy[1] == "world");

            CHECK(vec.size() == 2);
            CHECK(vec[0] == "hello");
            CHECK(vec[1] == "world");
        }

        THEN("copy assignment copies its elements") {
            vec.emplace_back("hello");
            vec.emplace_back("world");

            sm::SmallVector<std::string, 64> copy;
            copy.emplace_back("foo");

            copy = vec.clone();

            CHECK(copy.size() == 2);
            CHECK(copy[0] == "hello");
            CHECK(copy[1] == "world");

            CHECK(vec.size() == 2);
            CHECK(vec[0] == "hello");
            CHECK(vec[1] == "world");
        }

        THEN("moving the vector moves its elements") {
            vec.emplace_back("hello");
            vec.emplace_back("world");

            sm::SmallVector<std::string, 64> moved = std::move(vec);

            CHECK(moved.size() == 2);
            CHECK(moved[0] == "hello");
            CHECK(moved[1] == "world");

            CHECK(vec.size() == 0);
            CHECK(vec.isEmpty());
        }
    }

    GIVEN("a small vector") {
        sm::SmallVector<std::string, 2> vec;

        THEN("its capacity matches its static size") {
            CHECK(vec.capacity() == 2);
        }

        THEN("it cannot accept more than its capacity") {
            vec.emplace_back("hello");
            vec.emplace_back("world");

            CHECK_FALSE(vec.tryEmplaceBack("foo"));
        }

        THEN("it cannot be resized outside valid bounds") {
            CHECK(vec.tryResize(0));
            CHECK(vec.tryResize(2));
            CHECK_FALSE(vec.tryResize(3));
            CHECK_FALSE(vec.tryResize(-1));
        }

        THEN("elements cannot be popped from an empty vector") {
            CHECK_FALSE(vec.tryPopBack());
        }
    }

    GIVEN("a const vector") {
        const auto vec = [] {
            sm::SmallVector<std::string, 64> vec;
            vec.emplace_back("hello");
            vec.emplace_back("world");
            return vec;
        }();

        THEN("it is not empty") {
            CHECK_FALSE(vec.isEmpty());
            CHECK(vec.size() == 2);
            CHECK(vec.sizeInBytes() == 2 * sizeof(std::string));
        }

        THEN("it can be iterated over") {
            std::size_t i = 0;
            for (const auto &str : vec) {
                if (i == 0) CHECK(str == "hello");
                if (i == 1) CHECK(str == "world");
                ++i;
            }

            CHECK(i == 2);
        }

        THEN("it can be indexed") {
            CHECK(vec[0] == "hello");
            CHECK(vec[1] == "world");
        }

        THEN("it copies correctly") {
            sm::SmallVector<std::string, 64> copy = sm::SmallVector{vec};

            CHECK(copy.size() == 2);
            CHECK(copy[0] == "hello");
            CHECK(copy[1] == "world");

            CHECK(vec.size() == 2);
            CHECK(vec[0] == "hello");
            CHECK(vec[1] == "world");
        }
    }

    GIVEN("default initialization") {
        sm::SmallVector<int, 4> vec;

        THEN("it initializes correctly") {
            CHECK(vec.size() == 0);
            CHECK(vec.empty());
        }

        THEN("apendding elements works") {
            vec.push_back(1);
            vec.push_back(2);
            vec.push_back(3);

            CHECK(vec.size() == 3);
            CHECK_FALSE(vec.empty());
            CHECK(vec[0] == 1);
            CHECK(vec[1] == 2);
            CHECK(vec[2] == 3);
        }
    }

    GIVEN("initial values") {
        sm::SmallVector<int, 4> vec = { 1, 2, 3, 4 };

        THEN("it initializes correctly") {
            CHECK(vec.size() == 4);
            CHECK_FALSE(vec.empty());
            CHECK(vec[0] == 1);
            CHECK(vec[1] == 2);
            CHECK(vec[2] == 3);
            CHECK(vec[3] == 4);
        }
    }

    GIVEN("a new vector") {
        sm::SmallVector<int, 4> vec { 1, 2, 3, 4 };

        THEN("it can be moved") {
            sm::SmallVector<int, 4> moved = std::move(vec);

            CHECK(moved.size() == 4);
            CHECK_FALSE(moved.empty());
            CHECK(moved[0] == 1);
            CHECK(moved[1] == 2);
            CHECK(moved[2] == 3);
            CHECK(moved[3] == 4);
        }
    }

    GIVEN("a vector with many elements") {
        sm::SmallVector<int, 256> vec;
        for (int i = 0; i < 64; ++i) {
            vec.push_back(i);
        }

        THEN("it initializes correctly") {
            CHECK(vec.size() == 64);
            CHECK_FALSE(vec.empty());
            for (int i = 0; i < 64; ++i) {
                if (vec[i] != i) CHECK(vec[i] == i);
            }
        }

        THEN("it can be shrunk") {
            vec.resize(32);

            CHECK(vec.size() == 32);
            CHECK_FALSE(vec.empty());
            for (int i = 0; i < 32; ++i) {
                if (vec[i] != i) CHECK(vec[i] == i);
            }
        }

        THEN("it can be resized with a larger size") {
            vec.resize(64);

            CHECK(vec.size() == 64);
            CHECK_FALSE(vec.empty());
            for (int i = 0; i < 64; ++i) {
                if (vec[i] != i) CHECK(vec[i] == i);
            }
        }

        THEN("it can be cleared") {
            vec.clear();

            CHECK(vec.size() == 0);
            CHECK(vec.empty());
        }

        THEN("it can be iterated over") {
            int i = 0;
            for (int val : vec) {
                CHECK(val == i);
                ++i;
            }

            CHECK(i == 64);
        }
    }

    GIVEN("a vector with non copyable contents") {
        struct NonCopyable {
            NonCopyable() = default;
            NonCopyable(const NonCopyable &) = delete;
            NonCopyable(NonCopyable &&) = default;
            NonCopyable &operator=(const NonCopyable &) = delete;
            NonCopyable &operator=(NonCopyable &&) = default;
        };

        sm::SmallVector<NonCopyable, 4> vec;

        THEN("it initializes correctly") {
            CHECK(vec.size() == 0);
            CHECK(vec.empty());
        }

        THEN("apendding elements works") {
            vec.push_back(NonCopyable());
            vec.push_back(NonCopyable());
            vec.push_back(NonCopyable());

            CHECK(vec.size() == 3);
            CHECK_FALSE(vec.empty());
        }

        THEN("emplace works") {
            sm::SmallVector<NonCopyable, 4> vec2;

            vec2.emplace_back();
            vec2.emplace_back();
            vec2.emplace_back();

            CHECK(vec2.size() == 3);
            CHECK_FALSE(vec2.empty());
        }
    }
}