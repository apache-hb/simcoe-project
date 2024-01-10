#include "core/panic.hpp"

#include <iostream>

using namespace simcoe;

void simcoe::panic(const PanicInfo &info, std::string msg) {
    std::cerr << "panic[" << info.file << ":" << info.line << "] @ " << info.symbol << ": " << msg << std::endl;
    std::exit(SM_EXIT_INTERNAL);
}
