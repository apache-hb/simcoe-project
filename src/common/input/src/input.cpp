#include "input/input.hpp"

using namespace sm;
using namespace sm::input;

bool InputState::updateButton(Button button, size_t next) {
    bool updated = buttons[std::to_underlying(button)] != next;
    buttons[std::to_underlying(button)] = next;
    return updated;
}

float InputState::buttonAxis(ButtonAxis pair) const {
    size_t n = buttons[std::to_underlying(pair.negative)];
    size_t p = buttons[std::to_underlying(pair.positive)];

    if (n > p) return -1.f;
    if (p > n) return 1.f;
    else return 0.f;
}

math::float2 InputState::buttonAxis2d(ButtonAxis horizontal, ButtonAxis vertical) const {
    float h = buttonAxis(horizontal);
    float v = buttonAxis(vertical);
    return {h, v};
}

math::float3 InputState::buttonAxis3d(ButtonAxis horizontal, ButtonAxis vertical, ButtonAxis depth) const {
    float h = buttonAxis(horizontal);
    float v = buttonAxis(vertical);
    float d = buttonAxis(depth);
    return {h, v, d};
}

float InputState::buttonAxis(ButtonSetAxis pair) const {
    auto maxButtonOf = [&](const ButtonSet& set) -> size_t {
        size_t max = 0;
        for (Button button : set.buttons) {
            max = std::max(max, buttons[std::to_underlying(button)]);
        }
        return max;
    };

    size_t n = maxButtonOf(pair.negative);
    size_t p = maxButtonOf(pair.positive);

    if (n > p) return -1.f;
    if (p > n) return 1.f;
    else return 0.f;
}

math::float2 InputState::buttonAxis2d(ButtonSetAxis horizontal, ButtonSetAxis vertical) const {
    float h = buttonAxis(horizontal);
    float v = buttonAxis(vertical);
    return {h, v};
}

math::float3 InputState::buttonAxis3d(ButtonSetAxis horizontal, ButtonSetAxis vertical, ButtonSetAxis depth) const {
    float h = buttonAxis(horizontal);
    float v = buttonAxis(vertical);
    float d = buttonAxis(depth);
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

bool InputState::isButtonDown(Button button) const {
    return buttons[std::to_underlying(button)] != 0;
}

void InputService::addSource(ISource* source) {
    mSources.emplace(source);
}

void InputService::addClient(IClient* client) {
    mClients.emplace(client);
}

void InputService::eraseSource(ISource* source) {
    mSources.erase(source);
}

void InputService::eraseClient(IClient* client) {
    mClients.erase(client);
}

void InputService::poll() {
    bool dirty = false;
    for (ISource *source : mSources) {
        if (source->poll(mState)) {
            mState.device = source->getType();
            dirty = true;
        }
    }

    if (!dirty)
        return;

    for (IClient *client : mClients) {
        client->accept(mState, *this);
    }
}

void InputService::captureCursor(bool capture) {
    for (ISource *source : mSources) {
        source->captureCursor(capture);
    }
}
