#include "render/object.hpp"

using namespace sm;
using namespace sm::render;

std::string_view Blob::as_string() const {
    return {reinterpret_cast<const char*>(data()), size()};
}

const void *Blob::data() const {
    return get()->GetBufferPointer();
}

size_t Blob::size() const {
    return get()->GetBufferSize();
}
