#include "test/common.hpp"

#include "core/adt/vector.hpp"

TEST_CASE("vector operations") {
    GIVEN("default initialization") {
        sm::VectorBase<int> vec;

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
        sm::VectorBase<int> vec = { 1, 2, 3, 4 };

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
        sm::VectorBase<int> vec { 1, 2, 3, 4 };

        THEN("it can be moved") {
            sm::VectorBase<int> moved = std::move(vec);

            CHECK(moved.size() == 4);
            CHECK_FALSE(moved.empty());
            CHECK(moved[0] == 1);
            CHECK(moved[1] == 2);
            CHECK(moved[2] == 3);
            CHECK(moved[3] == 4);
        }
    }

    GIVEN("a vector with many elements") {
        sm::VectorBase<int> vec;
        for (int i = 0; i < 1000; ++i) {
            vec.push_back(i);
        }

        THEN("it initializes correctly") {
            CHECK(vec.size() == 1000);
            CHECK_FALSE(vec.empty());
            for (int i = 0; i < 1000; ++i) {
                CHECK(vec[i] == i);
            }
        }

        THEN("it can be resized") {
            vec.resize(500);

            CHECK(vec.size() == 500);
            CHECK_FALSE(vec.empty());
            for (int i = 0; i < 500; ++i) {
                CHECK(vec[i] == i);
            }
        }

        THEN("it can be resized with a larger size") {
            vec.resize(1500);

            CHECK(vec.size() == 1500);
            CHECK_FALSE(vec.empty());
            for (int i = 0; i < 1000; ++i) {
                CHECK(vec[i] == i);
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

            CHECK(i == 1000);
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

        sm::VectorBase<NonCopyable> vec;

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
            sm::VectorBase<NonCopyable> vec2;

            vec2.emplace_back();
            vec2.emplace_back();
            vec2.emplace_back();

            CHECK(vec2.size() == 3);
            CHECK_FALSE(vec2.empty());
        }
    }
}