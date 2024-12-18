#include "core/win32.hpp" // IWYU pragma: export

extern "C" {
	// agility sdk
	__declspec(dllexport) extern const UINT D3D12SDKVersion = 613;        // NOLINT
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\redist\\d3d12\\"; // NOLINT
}
