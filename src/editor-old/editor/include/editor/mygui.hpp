#pragma once

#include "math/math.hpp"

namespace MyGui {
    bool SliderAngle3(const char *label, sm::math::radf3 *value, sm::math::degf min, sm::math::degf max);
    bool InputAngle3(const char *label, sm::math::radf3 *value);
    bool DragAngle3(const char *label, sm::math::radf3 *value, sm::math::degf speed, sm::math::degf min, sm::math::degf max);

    bool EditSwizzle(const char *label, sm::uint8 *mask, int components);

    bool BeginPopupWindow(const char *title, ImGuiWindowFlags flags = 0);
}
