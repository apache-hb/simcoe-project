#include "input/input.hpp"

using namespace sm;
using namespace sm::input;

bool InputState::update_button(Button button, size_t next) {
    bool updated = buttons[button] != next;
    buttons[button] = next;
    return updated;
}

float InputState::button_axis(ButtonAxis pair) const {
    size_t n = buttons[pair.negative];
    size_t p = buttons[pair.positive];

    if (n > p) return -1.f;
    if (p > n) return 1.f;
    else return 0.f;
}

math::float2 InputState::button_axis2d(ButtonAxis horizontal, ButtonAxis vertical) const {
    float h = button_axis(horizontal);
    float v = button_axis(vertical);
    return {h, v};
}

float InputState::axis(Axis id) const {
    return axes[(size_t)id];
}

math::float2 InputState::axis2d(Axis horizontal, Axis vertical) const {
    float h = axis(horizontal);
    float v = axis(vertical);
    return {h, v};
}

void InputService::add_source(ISource* source) {
    m_sources.push_back(source);
}

void InputService::add_client(IClient* client) {
    m_clients.push_back(client);
}

void InputService::poll() {
    bool dirty = false;
    for (ISource *source : m_sources) {
        if (source->poll(m_state)) {
            m_state.device = source->get_type();
            dirty = true;
        }
    }

    if (!dirty)
        return;

    for (IClient *client : m_clients) {
        client->accept(m_state, *this);
    }
}

void InputService::capture_cursor(bool capture) {
    for (ISource *source : m_sources) {
        source->capture_cursor(capture);
    }
}
