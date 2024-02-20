#pragma once

#include "core/vector.hpp"
#include "core/array.hpp"

#include "math/math.hpp"

#include "input.reflect.h"

namespace sm::input {
    using ButtonState = sm::Array<size_t, Button::kCount>;
    using AxisState = sm::Array<float, Axis::kCount>;

    class InputService;

    struct ButtonAxis {
        Button negative;
        Button positive;
    };

    struct InputState {
        InputDevice device;
        ButtonState buttons;
        AxisState axes;

        bool update_button(Button button, size_t next);

        float button_axis(ButtonAxis pair) const;
        math::float2 button_axis2d(ButtonAxis horizontal, ButtonAxis vertical) const;

        float axis(Axis id) const;
        math::float2 axis2d(Axis horizontal, Axis vertical) const;
    };

    class ISource {
        InputDevice m_type;
    public:
        virtual ~ISource() = default;
        constexpr ISource(InputDevice type) : m_type(type) { }

        constexpr InputDevice get_type() const { return m_type; }

        virtual bool poll(InputState& state) = 0;
        virtual void capture_cursor(bool capture) { }
    };

    class IClient {
    public:
        virtual ~IClient() = default;

        virtual void accept(const InputState& state, InputService& service) = 0;
    };

    class InputService {
        sm::Vector<ISource*> m_sources;
        sm::Vector<IClient*> m_clients;

        InputState m_state{};

    public:
        constexpr InputService() = default;

        void add_source(ISource* source);
        void add_client(IClient* client);

        void capture_cursor(bool capture);

        void poll();

        const InputState& get_state() const { return m_state; }
    };
}
