#include "ui/control.hpp"

using namespace sm;
using namespace sm::ui;

NavControl::NavControl(Canvas& canvas, IWidget *initial)
    : m_canvas(canvas)
    , m_current(initial)
{
    m_current->begin_focus();
}

void NavControl::add_link(IWidget *widget, input::Button action, IWidget *target) {
    m_links.emplace(widget, NavLink{target, action});
}

void NavControl::add_bidi_link(IWidget *widget, input::Button to, IWidget *target, input::Button from) {
    add_link(widget, to, target);
    add_link(target, from, widget);
}

void NavControl::accept(const input::InputState& state) {
    auto range = m_links.equal_range(m_current);
    for (auto it = range.first; it != range.second; ++it) {
        auto [widget, link] = *it;
        if (state.buttons[link.action.as_integral()]) {
            m_current->end_focus();
            m_current = link.widget;
            m_current->begin_focus();

            m_canvas.layout();
            return;
        }
    }
}
