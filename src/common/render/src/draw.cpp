#include "render/draw.hpp"

using namespace sm;
using namespace sm::draw;

float4x4 Transform::matrix() const {
    return float4x4::transform(position, rotation, scale);
}
