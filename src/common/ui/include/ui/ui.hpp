#pragma once

#include "core/utf8.hpp"

#include "core/vector.hpp"
#include "math/math.hpp"

#include "render/render.hpp"

#include "ui.reflect.h"

// very barebones ui system
// text, buttons, layouts
namespace sm::ui {
    struct BoxBounds {
        math::float2 min;
        math::float2 max;
    };

    struct Align {
        AlignH h;
        AlignV v;
    };

    class IWidget {
        Align m_align;

    public:
        virtual ~IWidget() = default;

        Align get_align() const { return m_align; }
    };

    class TextWidget : public IWidget {
        utf8::StaticText m_text;
    };

    class ImageWidget : public IWidget {
        SM_UNUSED render::Texture m_texture;
    };

    class ButtonWidget : public IWidget {

    };

    class VStackWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
    };

    class HStackWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
    };

    class GridWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
    };

    struct Vertex {
        math::float2 position;
        math::float2 uv;
        math::uint8x4 colour;
    };

    struct CanvasDrawData {
        sm::Vector<Vertex> vertices;
        sm::Vector<uint16_t> indices;
    };

    class Canvas {
        // resolution of the entire screen
        BoxBounds m_screen;

        // resolution of the region we are allowed to draw in
        BoxBounds m_user;

    public:
        Canvas(BoxBounds screen, BoxBounds user)
            : m_screen(screen)
            , m_user(user)
        { }

        BoxBounds get_screen_bounds() const { return m_screen; }
        BoxBounds get_user_bounds() const { return m_user; }

        void layout(CanvasDrawData& data, const IWidget& widget);
    };
}
