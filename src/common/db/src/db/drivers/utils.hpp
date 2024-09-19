#pragma once

namespace sm::dao {
    struct TableInfo;
}

namespace sm::db::detail {
    size_t primaryKeyIndex(const dao::TableInfo& info) noexcept;
}
