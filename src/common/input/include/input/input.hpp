#pragma once

#include "math/math.hpp"

#include <unordered_set>
#include <array>
#include <span>

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

    using ButtonState = std::array<size_t, (int)Button::eCount>;
    using AxisState = std::array<float, (int)Axis::eCount>;

    class InputService;

    struct ButtonAxis {
        Button negative;
        Button positive;
    };

    struct ButtonSet {
        /// @pre there are no duplicates in the list
        std::span<Button> buttons;
    };

    struct ButtonSetAxis {
        ButtonSet negative;
        ButtonSet positive;
    };

    struct InputState {
        DeviceType device;
        ButtonState buttons;
        AxisState axes;

        bool updateButton(Button button, size_t next);

        float buttonAxis(ButtonAxis pair) const;
        math::float2 buttonAxis2d(ButtonAxis horizontal, ButtonAxis vertical) const;
        math::float3 buttonAxis3d(ButtonAxis horizontal, ButtonAxis vertical, ButtonAxis depth) const;

        float buttonAxis(ButtonSetAxis pair) const;
        math::float2 buttonAxis2d(ButtonSetAxis horizontal, ButtonSetAxis vertical) const;
        math::float3 buttonAxis3d(ButtonSetAxis horizontal, ButtonSetAxis vertical, ButtonSetAxis depth) const;

        float axis(Axis id) const;
        math::float2 axis2d(Axis horizontal, Axis vertical) const;

        bool isButtonDown(Button button) const;
    };

    class ISource {
        DeviceType mDeviceType;
    public:
        virtual ~ISource() = default;
        constexpr ISource(DeviceType type) : mDeviceType(type) { }

        constexpr DeviceType getType() const { return mDeviceType; }

        virtual bool poll(InputState& state) = 0;
        virtual void captureCursor(bool capture) { }
    };

    class IClient {
    public:
        virtual ~IClient() = default;

        virtual void accept(const InputState& state, InputService& service) = 0;
    };

    class InputService {
        std::unordered_set<ISource*> mSources;
        std::unordered_set<IClient*> mClients;

        InputState mState{};

    public:
        constexpr InputService() = default;

        void addSource(ISource* source);
        void addClient(IClient* client);

        void eraseSource(ISource* source);
        void eraseClient(IClient* client);

        void captureCursor(bool capture);

        void poll();

        const InputState& getState() const { return mState; }
    };
}
