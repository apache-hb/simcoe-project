#include "net_test_common.hpp"

NetTestStream::~NetTestStream() noexcept(false) {
    CHECK(errors.empty());
    for (const auto& error : errors) {
        FAIL_CHECK(error);
    }
}
