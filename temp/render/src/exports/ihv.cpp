#include "core/win32.hpp" // IWYU pragma: export

extern "C" {
	// ask for the discrete gpu on laptops
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                  // NOLINT
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}
