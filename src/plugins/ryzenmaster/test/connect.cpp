#include "ryzenmaster/driver.hpp"

#include <exception>
#include <iostream>

namespace rm = sm::amd::ryzenmaster;

int main() try {
    rm::DriverHandle handle{};
    handle.readMomentaryMemory();
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
}
