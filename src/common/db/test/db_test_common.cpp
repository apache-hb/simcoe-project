#include "db_test_common.hpp"

void checkError(const DbError& err) {
    if (!err.isSuccess()) {
        for (const auto& frame : err.stacktrace()) {
            fmt::println(stderr, "[{}:{}] {}", frame.source_file(), frame.source_line(), frame.description());
        }
        FAIL(err.message() << " (" << err.code() << ")");
    }
}

ConnectionConfig makeSqliteTestDb(const std::string& name, const ConnectionConfig& extra) {
    std::filesystem::path root = "test-data";
    std::filesystem::path path = root / (name + ".db");
    // create all directories up to the file
    std::filesystem::create_directories(path.parent_path());

    ConnectionConfig result = extra;
    result.host = path.string();
    return result;
}
