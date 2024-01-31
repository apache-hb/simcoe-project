#pragma once

#include "core/macros.hpp"
#include "core/utf8.hpp"

#include "bundle/bundle.hpp"
#include "core/vector.hpp"
#include "math/math.hpp"

#include "ui/atlas.hpp"

#include "ui.reflect.h"

// very barebones ui system
// text, buttons, layouts
namespace sm::ui {
    class Canvas;

    struct BoxBounds {
        // top left
        math::float2 min;

        // bottom right
        math::float2 max;
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

    struct LayoutInfo {
        CanvasDrawData& draw;
        Canvas& canvas;

        void box(const BoxBounds& bounds, const math::uint8x4& colour);
        void rect(const BoxBounds& bounds, float border, const math::uint8x4& colour);
    };

    struct Align {
        AlignH h = AlignH::eCenter;
        AlignV v = AlignV::eMiddle;
    };

    static constexpr math::uint8x4 kColourWhite = { 255, 255, 255, 255 };
    static constexpr math::uint8x4 kColourBlack = { 0, 0, 0, 255 };

    class IWidget {
        Align m_align;

    public:
        virtual ~IWidget() = default;

        virtual void layout(LayoutInfo& info, BoxBounds bounds) const { }

        Align get_align() const { return m_align; }
        void set_align(Align align) { m_align = align; }
    };

    class TextWidget : public IWidget {
        const FontAtlas& m_font;
        utf8::StaticText m_text;
        math::uint8x4 m_colour = kColourWhite;

    public:
        TextWidget(const FontAtlas& font, utf8::StaticText text)
            : m_font(font)
            , m_text(text)
        { }

        void layout(LayoutInfo& info, BoxBounds bounds) const override;

        const FontAtlas& get_font() const { return m_font; }
        const utf8::StaticText& get_text() const { return m_text; }

        TextWidget& text(utf8::StaticText text) { m_text = text; return *this; }
        TextWidget& colour(math::uint8x4 colour) { m_colour = colour; return *this; }
        TextWidget& align(Align align) { set_align(align); return *this; }
    };

    class TextureWidget : public IWidget {
        SM_UNUSED bundle::Image m_texture;
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

    class Canvas {
        bool m_dirty = false;
        CanvasDrawData m_draw;
        bundle::AssetBundle& m_bundle;
        FontAtlas& m_font;

        // resolution of the entire screen
        BoxBounds m_screen;

    public:
        Canvas(bundle::AssetBundle& bundle, FontAtlas& font)
            : m_bundle(bundle)
            , m_font(font)
        { }

        void set_screen(math::float2 min, math::float2 max) {
            m_screen = { min, max };
        }

        const BoxBounds& get_screen() const { return m_screen; }
        const math::float2& get_white_pixel_uv() const { return m_font.get_white_pixel_uv(); }

        bundle::AssetBundle& get_bundle() const { return m_bundle; }
        const CanvasDrawData& get_draw_data() const { return m_draw; }

        const FontAtlas& get_font() const { return m_font; }

        void layout(const IWidget& widget);

        bool is_dirty() const { return m_dirty; }
        void clear_dirty() { m_dirty = false; }


    };
}
