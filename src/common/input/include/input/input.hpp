#pragma once

#include "core/adt/vector.hpp"
#include "core/adt/array.hpp"

#include "math/math.hpp"

namespace sm::input {
    enum class DeviceType : uint8_t {
        eNone,
        eXInput,
        eDesktop,
        ePlayback,

        eCount
    };

    enum class Axis : uint8_t {
        eMouseX, eMouseY,

        eGamepadLeftX, eGamepadLeftY,
        eGamepadRightX, eGamepadRightY,
        eGamepadTriggerL, eGamepadTriggerR,

        eCount
    };

    enum class Button {
        ePadButtonUp,
        ePadButtonDown,
        ePadButtonLeft,
        ePadButtonRight,

        ePadDirectionUp,
        ePadDirectionDown,
        ePadDirectionLeft,
        ePadDirectionRight,

        ePadBumperL,
        ePadBumperR,

        ePadStickL,
        ePadStickR,

        eA, eB, eC, eD,
        eE, eF, eG, eH,
        eI, eJ, eK, eL,
        eM, eN, eO, eP,
        eQ, eR, eS, eT,
        eU, eV, eW, eX,
        eY, eZ,

        eKey0, eKey1, eKey2, eKey3, eKey4,
        eKey5, eKey6, eKey7, eKey8, eKey9,

        eShiftL,
        eShiftR,
        eCtrlL,
        eCtrlR,
        eAltL,
        eAltR,
        eSpace,
        eEnter,
        eBackspace,
        eTab,
        eEscape,
        eTilde,

        eMouseLeft,
        eMouseRight,
        eMouseMiddle,

        eMouseExtra1,
        eMouseExtra2,

        eKeyLeft,
        eKeyRight,
        eKeyUp,
        eKeyDown,

        eInvalid,

        eCount
    };

    using ButtonState = sm::Array<size_t, (int)Button::eCount>;
    using AxisState = sm::Array<float, (int)Axis::eCount>;

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
        math::float3 button_axis3d(ButtonAxis horizontal, ButtonAxis vertical, ButtonAxis depth) const;

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

        const InputState& getState() const { return mState; }
    };
}
