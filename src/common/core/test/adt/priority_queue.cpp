#include "test/common.hpp"

#include "core/adt/queue.hpp"

TEST_CASE("priority queue construction") {
    GIVEN("a default constructed priority queue") {
        sm::PriorityQueue<int> q;

        THEN("it initializes correctly") {
            CHECK(q.is_empty());
            CHECK(q.length() == 0);
        }
    }
}

TEST_CASE("priority queue operations") {
    GIVEN("a priority queue") {
        sm::PriorityQueue<int> q;

        THEN("pushing elements works") {
            q.emplace(1);
            q.emplace(2);
            q.emplace(3);

            CHECK(q.length() == 3);
            CHECK_FALSE(q.is_empty());
            CHECK(q[0] == 3);
            CHECK(q[1] == 1);
            CHECK(q[2] == 2);
        }

        q.clear();

        THEN("popping elements works") {
            q.emplace(1);
            q.emplace(2);
            q.emplace(3);

            CHECK(q.pop() == 3);
            CHECK(q.pop() == 2);
            CHECK(q.pop() == 1);
            CHECK(q.is_empty());
        }

        q.clear();

        THEN("top element is correct") {
            q.emplace(1);
            q.emplace(2);
            q.emplace(3);

            CHECK(q.top() == 3);
        }
    }

    GIVEN("a priority queue with custom comparator") {
        sm::PriorityQueue<int, std::greater<int>> q;

        THEN("pushing elements works") {
            q.emplace(1);
            q.emplace(2);
            q.emplace(3);

            CHECK(q.length() == 3);
            CHECK_FALSE(q.is_empty());
            CHECK(q[0] == 1);
            CHECK(q[1] == 2);
            CHECK(q[2] == 3);
        }

        q.clear();

        THEN("popping elements works") {
            q.emplace(1);
            q.emplace(2);
            q.emplace(3);

            CHECK(q.pop() == 1);
            CHECK(q.pop() == 2);
            CHECK(q.pop() == 3);
            CHECK(q.is_empty());
        }

        q.clear();

        THEN("top element is correct") {
            q.emplace(1);
            q.emplace(2);
            q.emplace(3);

            CHECK(q.top() == 1);
        }
    }

    GIVEN("a priority queue") {
        sm::PriorityQueue<int> q;

        THEN("elements inserted out of order are popped in order") {
            q.emplace(3);
            q.emplace(1);
            q.emplace(2);

            CHECK(q.pop() == 3);
            CHECK(q.pop() == 2);
            CHECK(q.pop() == 1);
        }
    }
}
