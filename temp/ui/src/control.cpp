#include "ui/control.hpp"

#include <utility>

using namespace sm;
using namespace sm::ui;

NavControl::NavControl(Canvas& canvas, IWidget *initial)
    : m_canvas(canvas)
    , m_current(initial)
{
    m_current->begin_focus();
}

NavLink& NavControl::add_link(IWidget *widget, input::Button action, IWidget *target) {
    auto it = m_links.emplace(widget, NavLink{target, action});
    return it->second;
}

NavAction& NavControl::add_action(IWidget *widget, input::Button button, std::function<void()> on_activate) {
    auto it = m_actions.emplace(widget, NavAction{button, std::move(on_activate)});
    return it->second;
}

void NavControl::add_bidi_link(IWidget *widget, input::Button to, IWidget *target, input::Button from) {
    add_link(widget, to, target);
    add_link(target, from, widget);
}

void NavControl::focus(IWidget *widget) {
    m_current->end_focus();
    m_current = widget;
    m_current->begin_focus();

    m_canvas.layout();
}

void NavControl::accept(const input::InputState& state) {
    auto actions = m_actions.equal_range(m_current);
    for (auto it = actions.first; it != actions.second; ++it) {
        auto [widget, action] = *it;
        if (state.buttons[action.button]) {
            action.on_activate();
        }
    }

    auto range = m_links.equal_range(m_current);
    for (auto it = range.first; it != range.second; ++it) {
        auto [widget, link] = *it;
        if (state.buttons[link.action]) {
            focus(link.widget);
            return;
        }
    }
}
