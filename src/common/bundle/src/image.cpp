#include "bundle/bundle.hpp"

using namespace sm;
using namespace sm::bundle;

uint8_t& Image::get_pixel(size_t x, size_t y, size_t channel) {
    size_t index = (y * size.width + x) * 4 + channel;
    return data[index];
}

math::uint8x4& Image::get_pixel_rgba(size_t x, size_t y) {
    using Reflect = ctu::TypeInfo<DataFormat>;
    CTASSERTF(format == DataFormat::eRGBA8_UINT, "image format is %s, not RGBA8_UINT", Reflect::to_string(format).data());
    size_t index = (y * size.width + x) * 4;
    return *reinterpret_cast<math::uint8x4*>(&data[index]);
}
