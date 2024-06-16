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

void *Library::get_symbol(const char *name) {
    void *result;
    mError = os_library_symbol(mHandle.address(), &result, name);

    if (mError.failed())
        return nullptr;

    return result;
}

OsError Library::get_error() const {
    return mError;
}
#endif
