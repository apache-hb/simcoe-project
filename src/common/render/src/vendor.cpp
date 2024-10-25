#include "core/win32.hpp" // IWYU pragma: export

#include <simcoe_render_config.h>

extern "C" {
	// ask for the discrete gpu on laptops
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                  // NOLINT
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // NOLINT
}

extern "C" {
#if SMC_RENDER_ENABLE_AGILITY
	// agility sdk
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 613;        // NOLINT
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\redist\\d3d12\\"; // NOLINT
#endif

	// dstorage, TODO: doesnt work, something about the core dll and regular dll
	__declspec(dllexport) extern const char *DStorageSDKPath = ".\\redist\\"; // NOLINT
}
