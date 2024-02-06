#include "bundle/bundle.hpp"
#include "logs/panic.hpp"
#include "core/reflect.hpp"

using namespace sm;
using namespace sm::bundle;

uint8_t& Image::get_pixel(size_t x, size_t y, size_t channel) {
    size_t index = (y * size.width + x) * 4 + channel;
    return data[index];
}

math::uint8x4& Image::get_pixel_rgba(size_t x, size_t y) {
    SM_ASSERTF(format == DataFormat::eRGBA8_UINT, "image format is {}, not RGBA8_UINT", format);
    size_t index = (y * size.width + x) * 4;
    return *reinterpret_cast<math::uint8x4*>(&data[index]);
}
