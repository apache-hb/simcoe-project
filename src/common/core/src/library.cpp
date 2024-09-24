#include "stdafx.hpp"

#ifdef _WIN32
#include "core/library.hpp"

using namespace sm;

Library::Library(const char *path) {
    os_library_t library;
    mError = os_library_open(path, &library);
    mHandle.reset(library);
}

Library::Library(const fs::path& path)
    : Library(path.string().c_str())
{ }

std::expected<Library, OsError> Library::open(const fs::path& path) noexcept {
    os_library_t library;
    OsError error = os_library_open(path.string().c_str(), &library);
    if (error.failed())
        return std::unexpected{error};

    return Library{library};
}

void *Library::getSymbol(const char *name) noexcept {
    void *result;
    mError = os_library_symbol(mHandle.address(), &result, name);

    if (mError.failed())
        return nullptr;

    return result;
}

OsError Library::getError() const noexcept {
    return mError;
}
#endif
