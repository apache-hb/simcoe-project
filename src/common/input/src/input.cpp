#include "input/input.hpp"

using namespace sm;
using namespace sm::input;

bool InputState::update_button(Button button, size_t next) {
    bool updated = buttons[std::to_underlying(button)] != next;
    buttons[std::to_underlying(button)] = next;
    return updated;
}

float InputState::button_axis(ButtonAxis pair) const {
    size_t n = buttons[std::to_underlying(pair.negative)];
    size_t p = buttons[std::to_underlying(pair.positive)];

    if (n > p) return -1.f;
    if (p > n) return 1.f;
    else return 0.f;
}

math::float2 InputState::button_axis2d(ButtonAxis horizontal, ButtonAxis vertical) const {
    float h = button_axis(horizontal);
    float v = button_axis(vertical);
    return {h, v};
}

math::float3 InputState::button_axis3d(ButtonAxis horizontal, ButtonAxis vertical, ButtonAxis depth) const {
    float h = button_axis(horizontal);
    float v = button_axis(vertical);
    float d = button_axis(depth);
    return {h, v, d};
}

float InputState::axis(Axis id) const {
    return axes[std::to_underlying(id)];
}

math::float2 InputState::axis2d(Axis horizontal, Axis vertical) const {
    float h = axis(horizontal);
    float v = axis(vertical);
    return {h, v};
}

bool InputState::is_button_down(Button button) const {
    return buttons[std::to_underlying(button)] != 0;
}

void InputService::add_source(ISource* source) {
    mSources.push_back(source);
}

void InputService::add_client(IClient* client) {
    mClients.push_back(client);
}

void InputService::erase_source(ISource* source) {
    mSources.erase(std::remove(mSources.begin(), mSources.end(), source), mSources.end());
}

void InputService::erase_client(IClient* client) {
    mClients.erase(std::remove(mClients.begin(), mClients.end(), client), mClients.end());
}

void InputService::poll() {
    bool dirty = false;
    for (ISource *source : mSources) {
        if (source->poll(mState)) {
            mState.device = source->get_type();
            dirty = true;
        }
    }

    if (!dirty)
        return;

    for (IClient *client : mClients) {
        client->accept(mState, *this);
    }
}

void InputService::capture_cursor(bool capture) {
    for (ISource *source : mSources) {
        source->capture_cursor(capture);
    }
}
