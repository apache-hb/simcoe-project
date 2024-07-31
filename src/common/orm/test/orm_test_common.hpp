#include "test/common.hpp"

// IWYU pragma: begin_exports
#include "orm/connection.hpp"
#include "orm/transaction.hpp"
// IWYU pragma: end_exports

using namespace sm;
using namespace sm::db;

void checkError(const DbError& err) {
    if (!err.isSuccess()) {
        FAIL(err.message() << " (" << err.code() << ")");
    }
}

template<typename T>
auto getValue(std::expected<T, DbError> result) {
    if (result.has_value())
        return std::move(result.value());

    checkError(result.error());
    std::unreachable();
}
