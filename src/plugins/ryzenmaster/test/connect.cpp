#include "ryzenmaster/driver.hpp"

#include <exception>
#include <iostream>

namespace rm = sm::amd::ryzenmaster;

int main() try {
    rm::DriverHandle handle{};
    handle.readPciConfig(0, 0x18, 1, 0x300, 4);
    handle.readPciConfig(0, 0x18, 1, 0x140, 4);
    handle.readPciConfig(0, 0x18, 1, 0x144, 4);
    handle.readPciConfig(0, 0x18, 0, 200, 0);
    // handle.readMomentaryMemory(0x5d0);

    // handle.readMemory();
    for (int i = 0; i < 16; i++) {
        try {
            handle.readMsrData(i);
        } catch (const std::exception &e) {
            std::cerr << "Error " << i << ": " << e.what() << '\n';
        }
    }

    handle.readMmioConfig();
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << '\n';
    return 1;
}
