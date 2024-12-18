#include "core/win32.hpp" // IWYU pragma: export

extern "C" {
	// dstorage, TODO: doesnt work, something about the core dll and regular dll
	__declspec(dllexport) extern const char *DStorageSDKPath = ".\\redist\\"; // NOLINT
}
