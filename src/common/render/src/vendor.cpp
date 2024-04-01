#include "core/win32.hpp" // IWYU pragma: export

extern "C" {
	// ask for the discrete gpu on laptops
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                  // NOLINT
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}

extern "C" {
	// agility sdk
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 613;        // NOLINT
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\redist\\d3d12\\"; // NOLINT

	// dstorage, TODO: doesnt work. possible bug in dstorage
	__declspec(dllexport) extern const char *DStorageSDKPath = ".\\redist\\"; // NOLINT
}
