#pragma once

#include "core/map.hpp"

#include "input/input.hpp"

#include "ui/ui.hpp"

namespace sm::ui {
    struct NavLink {
        IWidget *widget;
        input::Button action;
    };

    class NavControl : public input::IClient {
        sm::MultiMap<IWidget*, NavLink> m_links;

        Canvas& m_canvas;
        IWidget *m_current = nullptr;

    public:
        NavControl(Canvas& canvas, IWidget *initial);

        void add_link(IWidget *widget, input::Button action, IWidget *target);

        // equivalent to
        // add_link(widget, to, target)
        // add_link(target, from, widget)
        void add_bidi_link(IWidget *widget, input::Button to, IWidget *target, input::Button from);

        void accept(const input::InputState& state) override;
    };
}
