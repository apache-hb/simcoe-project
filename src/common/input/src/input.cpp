#include "input/input.hpp"

using namespace sm;
using namespace sm::input;

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
        client->accept(m_state);
    }
}
