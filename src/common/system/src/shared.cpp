#include "system/system.hpp"

using namespace sm;

std::string system::getMachineId() {
    char buffer[kMachineIdSize];
    system::getMachineId(buffer);
    return std::string(buffer, kMachineIdSize);
}
