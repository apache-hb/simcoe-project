#pragma once

#include <array>

#include "core/vector.hpp"

#include "input.reflect.h"

namespace sm::input {
    using ButtonState = std::array<size_t, size_t(Button::eCount)>;
    using AxisState = std::array<float, size_t(Axis::eCount)>;

    struct InputState {
        InputDevice device;
        ButtonState buttons;
        AxisState axes;
    };

    class ISource {
        InputDevice m_type;
    public:
        virtual ~ISource() = default;
        constexpr ISource(InputDevice type) : m_type(type) { }

        constexpr InputDevice get_type() const { return m_type; }

        virtual bool poll(InputState& state) = 0;
    };

    class IClient {
    public:
        virtual ~IClient() = default;

        virtual void accept(const InputState& state) = 0;
    };

    class InputService {
        sm::Vector<ISource*> m_sources;
        sm::Vector<IClient*> m_clients;

        InputState m_state{};

    public:
        constexpr InputService() = default;

        void add_source(ISource* source);
        void add_client(IClient* client);

        void poll();

        const InputState& get_state() const { return m_state; }
    };
}
