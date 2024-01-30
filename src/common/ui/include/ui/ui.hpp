#pragma once

#include "core/macros.hpp"
#include "core/utf8.hpp"

#include "bundle/bundle.hpp"
#include "core/vector.hpp"
#include "math/math.hpp"

#include "ui.reflect.h"

// very barebones ui system
// text, buttons, layouts
namespace sm::ui {
    class Canvas;

    struct FontAtlas {
        sm::Vector<char32_t> codepoints;
    };

    struct BoxBounds {
        math::float2 min;
        math::float2 max;
    };

    struct Align {
        AlignH h = AlignH::eCenter;
        AlignV v = AlignV::eMiddle;
    };

    class IWidget {
        Align m_align;

    public:
        virtual ~IWidget() = default;

        Align get_align() const { return m_align; }
        void set_align(Align align) { m_align = align; }
    };

    class TextWidget : public IWidget {
        SM_UNUSED FontAtlas& m_font;
        utf8::StaticText m_text;

    public:
        TextWidget(FontAtlas& font, utf8::StaticText text)
            : m_font(font)
            , m_text(text)
        { }
    };

    class TextureWidget : public IWidget {
        SM_UNUSED bundle::Texture m_texture;
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

    using Index = uint16_t;

    struct CanvasDrawData {
        sm::Vector<Vertex> vertices;
        sm::Vector<Index> indices;
    };

    class Canvas {
        bundle::AssetBundle& m_bundle;

        // resolution of the entire screen
        BoxBounds m_screen;

        // resolution of the region we are allowed to draw in
        BoxBounds m_user;

    public:
        Canvas(bundle::AssetBundle& bundle)
            : m_bundle(bundle)
        { }

        BoxBounds get_screen_bounds() const { return m_screen; }
        BoxBounds get_user_bounds() const { return m_user; }
        bundle::AssetBundle& get_bundle() const { return m_bundle; }

        void layout(CanvasDrawData& data, const IWidget& widget);

        bool is_dirty() const { return false; }
    };
}
