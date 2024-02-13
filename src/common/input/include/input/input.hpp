#pragma once

#include "core/vector.hpp"
#include "core/array.hpp"

#include "input.reflect.h"

namespace sm::input {
    using ButtonState = sm::Array<size_t, Button::kCount>;
    using AxisState = sm::Array<float, Axis::kCount>;

    struct InputState {
        InputDevice device;
        ButtonState buttons;
        AxisState axes;

        constexpr bool update_button(Button button, size_t next) {
            bool updated = buttons[button] != next;
            buttons[button] = next;
            return updated;
        }
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
