#include "test/common.hpp"

#include "account/salt.hpp"

#include <fstream>

TEST_CASE("Password salting") {
    std::mt19937 rng{9999};
    std::uniform_int_distribution<size_t> dist(0, 255);

    game::Salt salt{1234};

    for (size_t i = 0; i < 100; i++) {
        size_t size = dist(rng);
        auto salt1 = salt.getSaltString(size);

        CHECK(salt1.size() == size);
        for (size_t j = 0; j < size; j++) {
            if (!std::isprint(salt1[j])) {
                std::ofstream of{"salt.bin", std::ios::binary};
                of.write(salt1.data(), salt1.size());
                FAIL("Salt contains non-printable character at: " << j << " iteration: " << i << " size: " << size);
            }
        }
    }
}
