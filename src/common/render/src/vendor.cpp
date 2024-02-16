#include "render/render.hpp" // IWYU pragma: keep

extern "C" {
	// ask for the discrete gpu on laptops
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                  // NOLINT
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}

extern "C" {
	// agility sdk
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 611;        // NOLINT
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; // NOLINT
}