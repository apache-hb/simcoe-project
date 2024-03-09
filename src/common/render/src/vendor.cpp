#include "core/win32.hpp" // IWYU pragma: export

extern "C" {
	// ask for the discrete gpu on laptops
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                  // NOLINT
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}

extern "C" {
	// agility sdk
	// TODO: find a solution for client.exe.local at the point editor and client are seperate projects
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 611;        // NOLINT
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\client.exe.local\\d3d12\\"; // NOLINT
}
