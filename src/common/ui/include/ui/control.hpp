#pragma once

#include "core/map.hpp"

#include "input/input.hpp"

#include "ui/ui.hpp"

#include <functional>

namespace sm::ui {
    struct NavLink {
        IWidget *widget;
        input::Button action;

        std::function<void()> on_activate;
    };

    struct NavAction {
        input::Button button;
        std::function<void()> on_activate;
    };

    class NavControl : public input::IClient {
        sm::MultiMap<IWidget*, NavLink> m_links;
        sm::MultiMap<IWidget*, NavAction> m_actions;

        Canvas& m_canvas;
        IWidget *m_current = nullptr;

    public:
        NavControl(Canvas& canvas, IWidget *initial);

        NavLink& add_link(IWidget *widget, input::Button action, IWidget *target);
        NavAction& add_action(IWidget *widget, input::Button button, std::function<void()> on_activate);

        // equivalent to
        // add_link(widget, to, target)
        // add_link(target, from, widget)
        void add_bidi_link(IWidget *widget, input::Button to, IWidget *target, input::Button from);

        void accept(const input::InputState& state) override;
    };
}
