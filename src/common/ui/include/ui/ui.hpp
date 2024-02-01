#pragma once

#include "core/macros.hpp"
#include "core/utf8.hpp"
#include "core/debug.hpp"

#include "bundle/bundle.hpp"
#include "core/vector.hpp"
#include "math/math.hpp"

#include "ui/atlas.hpp"

#include "ui.reflect.h" // IWYU pragma: export

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
        void rounded_box(const BoxBounds& bounds, float rounding, const math::uint8x4& colour);
        void rect(const BoxBounds& bounds, float border, const math::uint8x4& colour);
    };

    struct LayoutResult {
        // bounds of the drawn object
        BoxBounds bounds;

        // offset into the vertex and index buffers
        size_t vtx_offset;
        size_t idx_offset;

        // number of vertices and indices used
        size_t vtx_count;
        size_t idx_count;

        void offset(CanvasDrawData& draw, math::float2 offset);
    };

    using LayoutResultList = sm::Vector<LayoutResult>;
    using LayoutResultSpan = std::span<LayoutResult>;

    static constexpr math::uint8x4 kColourWhite = { 255, 255, 255, 255 };
    static constexpr math::uint8x4 kColourBlack = { 0, 0, 0, 255 };
    static constexpr math::uint8x4 kColourRed = { 255, 0, 0, 255 };
    static constexpr math::uint8x4 kColourGreen = { 0, 255, 0, 255 };
    static constexpr math::uint8x4 kColourBlue = { 0, 0, 255, 255 };

    class IWidget {
        Align m_align;
        Padding m_padding;

        bool m_focus = false;

        SM_DBG_MEMBER(bool) m_draw_bounds = false;
        SM_DBG_MEMBER(math::uint8x4) m_dbg_colour = kColourRed;

    protected:
        // returns the bounds of the drawn object
        virtual BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const = 0;

        virtual void enter_focus() { }
        virtual void leave_focus() { }

    public:
        virtual ~IWidget() = default;

        LayoutResult layout(LayoutInfo& info, BoxBounds bounds) const;

        void begin_focus();
        void end_focus();

        Align get_align() const { return m_align; }
        void set_align(Align align) { m_align = align; }

        Padding get_padding() const { return m_padding; }
        void set_padding(Padding padding) { m_padding = padding; }

        constexpr bool has_focus() const { return m_focus; }

        constexpr bool draw_bounds() const { return SM_DBG_MEMBER_OR(m_draw_bounds, false); }
        constexpr math::uint8x4 dbg_colour() const { return SM_DBG_MEMBER_OR(m_dbg_colour, kColourRed); }

        void set_debug_draw(bool draw, math::uint8x4 colour) {
            SM_DBG_REF(m_draw_bounds) = draw;
            SM_DBG_REF(m_dbg_colour) = colour;
        }
    };

    class StyleWidget : public IWidget {
        IWidget& m_child;

        math::uint8x4 m_colour = kColourBlack;
        math::uint8x4 m_focus_colour = kColourGreen;

        float m_rounding = 0.f;

        const math::uint8x4& get_colour() const { return has_focus() ? m_focus_colour : m_colour; }

        void enter_focus() override { m_child.begin_focus(); }
        void leave_focus() override { m_child.end_focus(); }

        BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const override;

    public:
        StyleWidget(IWidget& child)
            : m_child(child)
        { }

        StyleWidget& colour(math::uint8x4 colour) { m_colour = colour; return *this; }
        StyleWidget& focus_colour(math::uint8x4 colour) { m_focus_colour = colour; return *this; }
        StyleWidget& align(Align align) { set_align(align); return *this; }
        StyleWidget& padding(Padding padding) { set_padding(padding); return *this; }
        StyleWidget& rounding(float rounding) { m_rounding = rounding; return *this; }
    };

    class TextWidget : public IWidget {
        const FontAtlas& m_font;
        utf8::StaticText m_text;
        math::uint8x4 m_colour = kColourWhite;
        math::uint8x4 m_focus_colour = kColourGreen;

        // use a scale rather than changing the size of the font
        float m_scale = 1.f;

        BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const override;

        const math::uint8x4& get_colour() const { return has_focus() ? m_focus_colour : m_colour; }

    public:
        TextWidget(const FontAtlas& font, utf8::StaticText text)
            : m_font(font)
            , m_text(text)
        { }

        const FontAtlas& get_font() const { return m_font; }
        const utf8::StaticText& get_text() const { return m_text; }

        TextWidget& text(utf8::StaticText text) { m_text = text; return *this; }
        TextWidget& scale(float scale) { m_scale = scale; return *this; }
        TextWidget& colour(math::uint8x4 colour) { m_colour = colour; return *this; }
        TextWidget& focus_colour(math::uint8x4 colour) { m_focus_colour = colour; return *this; }
        TextWidget& align(Align align) { set_align(align); return *this; }
        TextWidget& padding(Padding padding) { set_padding(padding); return *this; }
    };

    // swap out the child widget from a list of widgets
    class SwapControlWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
        size_t m_index = 0;

        BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const override;

    public:
        SwapControlWidget& add(const IWidget& widget, size_t& index) {
            m_children.push_back(&widget);
            index = m_children.size() - 1;
            return *this;
        }

        SwapControlWidget& index(size_t index) {
            m_index = index;
            return *this;
        }
    };

    // vertical stack of widgets
    class ZStackWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;

        BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const override;
    public:
        ZStackWidget& add(const IWidget& widget) {
            m_children.push_back(&widget);
            return *this;
        }

        ZStackWidget& align(Align align) {
            set_align(align);
            return *this;
        }
    };

    class TextureWidget : public IWidget {
        SM_UNUSED bundle::Image m_texture;
    };

    class ButtonWidget : public IWidget {

    };

    class VStackWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
        float m_spacing = 0.f;
        Justify m_justify = Justify::eFollowAlign;

        BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const override;
    public:
        VStackWidget& add(const IWidget& widget) {
            m_children.push_back(&widget);
            return *this;
        }

        VStackWidget& padding(Padding padding) {
            set_padding(padding);
            return *this;
        }

        VStackWidget& spacing(float padding) {
            m_spacing = padding;
            return *this;
        }

        VStackWidget& justify(Justify justify) {
            m_justify = justify;
            return *this;
        }

        VStackWidget& align(Align align) {
            set_align(align);
            return *this;
        }
    };

    class HStackWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
        float m_spacing = 0.f;
        Justify m_justify = Justify::eFollowAlign;

        BoxBounds impl_layout(LayoutInfo& info, BoxBounds bounds) const override;

        void justify_left(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const;
        void justify_right(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const;
        void justify_center(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const;

        void justify_fill(CanvasDrawData& draw, LayoutResultSpan results, const BoxBounds& bounds) const;
    public:
        HStackWidget& add(const IWidget& widget) {
            m_children.push_back(&widget);
            return *this;
        }

        HStackWidget& padding(Padding padding) {
            set_padding(padding);
            return *this;
        }

        HStackWidget& spacing(float padding) {
            m_spacing = padding;
            return *this;
        }

        HStackWidget& justify(Justify justify) {
            m_justify = justify;
            return *this;
        }

        HStackWidget& align(Align align) {
            set_align(align);
            return *this;
        }
    };

    class GridWidget : public IWidget {
        sm::Vector<const IWidget*> m_children;
    };

    class Canvas {
        bool m_need_repaint = false;
        CanvasDrawData m_draw;
        bundle::AssetBundle& m_bundle;
        FontAtlas& m_font;

        // resolution of the entire screen
        BoxBounds m_screen = { { 0.f, 0.f }, { 0.f, 0.f } };

        // user region, relative to screen bounds
        BoxBounds m_user = { { 0.f, 0.f }, { 0.f, 0.f } };

        IWidget& m_root;

    public:
        Canvas(bundle::AssetBundle& bundle, FontAtlas& font, IWidget& root)
            : m_bundle(bundle)
            , m_font(font)
            , m_root(root)
        { }

        void set_screen(math::uint2 size) {
            m_screen = { { 0.f, 0.f }, { float(size.x), float(size.y) } };
        }

        void set_user(math::float2 min, math::float2 max) {
            m_user = { min, max };
        }

        const BoxBounds& get_screen() const { return m_screen; }
        constexpr BoxBounds get_user() const {
            auto min = m_screen.min + m_user.min;
            auto max = m_screen.max + m_user.max;
            return { min, max };
        }

        const math::float2& get_white_pixel_uv() const { return m_font.get_white_pixel_uv(); }

        bundle::AssetBundle& get_bundle() const { return m_bundle; }
        const CanvasDrawData& get_draw_data() const { return m_draw; }

        const FontAtlas& get_font() const { return m_font; }

        void layout();

        bool needs_repaint() const { return m_need_repaint; }
        void clear_repaint() { m_need_repaint = false; }
    };
}
