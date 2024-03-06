#include "core/library.hpp"

using namespace sm;

Library::Library(const char *path) {
    os_library_t library;
    mError = os_library_open(path, &library);
    LibraryHandle::reset(library);
}

Library::Library(const fs::path& path)
    : Library(path.string().c_str())
{ }

os_symbol_t Library::get_symbol(const char *name) {
    os_symbol_t result;
    mError = os_library_symbol(LibraryHandle::address(), &result, name);

    if (mError.failed())
        return nullptr;

    return result;
}

OsError Library::get_error() const {
    return mError;
}
