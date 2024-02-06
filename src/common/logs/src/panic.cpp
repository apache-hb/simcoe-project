#include "logs/panic.hpp"

using namespace sm;

CT_NORETURN sm::panic(source_info_t info, std::string_view msg) {
    ctu_panic(info, "%.*s", (int)msg.size(), msg.data());
}
