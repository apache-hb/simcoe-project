#include "test/common.hpp"

// IWYU pragma: begin_exports
#include "orm/connection.hpp"
#include "orm/transaction.hpp"
// IWYU pragma: end_exports

using namespace sm;
using namespace sm::db;

void checkError(const DbError& err) {
    CTASSERTF(err.isSuccess(), "Failed to get value: %s", err.message().data());
}

template<typename T>
auto getValue(std::expected<T, DbError> result) {
    if (result.has_value())
        return std::move(result.value());

    CT_NEVER("Failed to get value: %s", result.error().message().data());
}
