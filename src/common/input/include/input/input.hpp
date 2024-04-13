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
        DeviceType device;
        ButtonState buttons;
        AxisState axes;

        bool update_button(Button button, size_t next);

        float button_axis(ButtonAxis pair) const;
        math::float2 button_axis2d(ButtonAxis horizontal, ButtonAxis vertical) const;

        float axis(Axis id) const;
        math::float2 axis2d(Axis horizontal, Axis vertical) const;

        bool is_button_down(Button button) const;
    };

    class ISource {
        DeviceType mDeviceType;
    public:
        virtual ~ISource() = default;
        constexpr ISource(DeviceType type) : mDeviceType(type) { }

        constexpr DeviceType get_type() const { return mDeviceType; }

        virtual bool poll(InputState& state) = 0;
        virtual void capture_cursor(bool capture) { }
    };

    class IClient {
    public:
        virtual ~IClient() = default;

        virtual void accept(const InputState& state, InputService& service) = 0;
    };

    class InputService {
        sm::Vector<ISource*> mSources;
        sm::Vector<IClient*> mClients;

        InputState mState{};

    public:
        constexpr InputService() = default;

        void add_source(ISource* source);
        void add_client(IClient* client);

        void erase_source(ISource* source);
        void erase_client(IClient* client);

        void capture_cursor(bool capture);

        void poll();

        const InputState& get_state() const { return mState; }
    };
}
