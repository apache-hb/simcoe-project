#include "system/input.hpp"

using namespace sm;

using DesktopInput = system::DesktopInput;

// handling keyboard accuratley is more tricky than it first seems
// https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
static DWORD remapKeyCode(WPARAM wparam, LPARAM lparam) {
    WORD vk = LOWORD(wparam);
    WORD flags = HIWORD(lparam);
    WORD scancode = LOBYTE(flags);

    if ((flags & KF_EXTENDED) == KF_EXTENDED) {
        scancode = MAKEWORD(scancode, 0xE0);
    }

    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) {
        vk = LOWORD(MapVirtualKeyA(scancode, MAPVK_VSC_TO_VK_EX));
    }

    return vk;
}

struct ButtonMap { unsigned key; input::Button button; };

template<size_t N>
static consteval auto makeTable(const ButtonMap(&data)[N]) {
    Array<input::Button, 256> table{};
    for (const auto& entry : data) {
        table[entry.key] = entry.button;
    }
    return table;
}

static constexpr auto kButtons = makeTable({
    { 'A', input::Button::eA },
    { 'B', input::Button::eB },
    { 'C', input::Button::eC },
    { 'D', input::Button::eD },
    { 'E', input::Button::eE },
    { 'F', input::Button::eF },
    { 'G', input::Button::eG },
    { 'H', input::Button::eH },
    { 'I', input::Button::eI },
    { 'J', input::Button::eJ },
    { 'K', input::Button::eK },
    { 'L', input::Button::eL },
    { 'M', input::Button::eM },
    { 'N', input::Button::eN },
    { 'O', input::Button::eO },
    { 'P', input::Button::eP },
    { 'Q', input::Button::eQ },
    { 'R', input::Button::eR },
    { 'S', input::Button::eS },
    { 'T', input::Button::eT },
    { 'U', input::Button::eU },
    { 'V', input::Button::eV },
    { 'W', input::Button::eW },
    { 'X', input::Button::eX },
    { 'Y', input::Button::eY },
    { 'Z', input::Button::eZ },

    { '0', input::Button::eKey0 },
    { '1', input::Button::eKey1 },
    { '2', input::Button::eKey2 },
    { '3', input::Button::eKey3 },
    { '4', input::Button::eKey4 },
    { '5', input::Button::eKey5 },
    { '6', input::Button::eKey6 },
    { '7', input::Button::eKey7 },
    { '8', input::Button::eKey8 },
    { '9', input::Button::eKey9 },

    { VK_LEFT,  input::Button::eKeyLeft },
    { VK_RIGHT, input::Button::eKeyRight },
    { VK_UP,    input::Button::eKeyUp },
    { VK_DOWN,  input::Button::eKeyDown },

    { VK_ESCAPE, input::Button::eEscape },
    { VK_LSHIFT, input::Button::eShiftL },
    { VK_RSHIFT, input::Button::eShiftR },

    { VK_LCONTROL, input::Button::eCtrlL },
    { VK_RCONTROL, input::Button::eCtrlR },
    { VK_LMENU, input::Button::eAltL },
    { VK_RMENU, input::Button::eAltR },
    { VK_SPACE, input::Button::eSpace },

    { VK_RETURN, input::Button::eEnter },

    { VK_OEM_3, input::Button::eTilde },

    { VK_LBUTTON, input::Button::eMouseLeft },
    { VK_RBUTTON, input::Button::eMouseRight },
    { VK_MBUTTON, input::Button::eMouseMiddle },

    { VK_XBUTTON1, input::Button::eMouseExtra1 },
    { VK_XBUTTON2, input::Button::eMouseExtra2 }
});

template<typename T>
static bool update(T& value, T next) {
    bool bUpdated = value != next;
    value = next;
    return bUpdated;
}

static math::int2 getMousePosition(HWND hwnd) {
    POINT p;
    SM_ASSERT_WIN32(GetCursorPos(&p));
    return {p.x, p.y};
}

static math::int2 getWindowCenter(system::Window& hwnd) {
    RECT rect;
    SM_ASSERT_WIN32(GetWindowRect(hwnd.get_handle(), &rect));

    return {
        (rect.left + rect.right) / 2,
        (rect.top + rect.bottom) / 2
    };
}

void DesktopInput::setKeyValue(WORD key, size_t value) {
    if (auto button = kButtons[key]; button != input::Button::eInvalid) {
        mButtons[std::to_underlying(button)] = value;
    }
}

void DesktopInput::setXButtonValue(WORD key, size_t value) {
    switch (key) {
    case XBUTTON1:
        setKeyValue(VK_XBUTTON1, value);
        break;
    case XBUTTON2:
        setKeyValue(VK_XBUTTON2, value);
        break;
    default:
        setKeyValue(key, value);
        break;
    }
}

void DesktopInput::centerMouse() {
    auto center = getWindowCenter(mWindow);
    SetCursorPos(center.x, center.y);
}

static constexpr auto kMouseX = (size_t)input::Axis::eMouseX;
static constexpr auto kMouseY = (size_t)input::Axis::eMouseY;

bool DesktopInput::pollMouseState(input::InputState& state) {
    auto pos = getMousePosition(mWindow.get_handle());

    // TODO: maybe add this back in if the ecs system doesnt work out
    // if (pos == mMousePosition) return false;

    auto delta = pos - mMousePosition;

    if (mCaptureMouse) {
        centerMouse();
    } else {
        mMousePosition = pos;
    }

    state.axes[kMouseX] = -float(delta.x);
    state.axes[kMouseY] = -float(delta.y);

    return true;
}

DesktopInput::DesktopInput(system::Window& window)
    : ISource(input::DeviceType::eDesktop)
    , mWindow(window)
    , mMousePosition(getMousePosition(window.get_handle()))
{ }

bool DesktopInput::poll(input::InputState& state) {
    bool dirty = pollMouseState(state);

    for (unsigned i = 0; i < kButtons.length(); ++i) {
        if (auto button = kButtons[i]; button != input::Button::eInvalid) {
            dirty |= update(state.buttons[std::to_underlying(button)], mButtons[std::to_underlying(button)]);
        }
    }

    return dirty;
}

void DesktopInput::window_event(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        // ignore key repeats
        if (HIWORD(lparam) & KF_REPEAT) { return; }

        setKeyValue(remapKeyCode(wparam, lparam), mIndex++);
        break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
        setKeyValue(remapKeyCode(wparam, lparam), 0);
        break;

    case WM_RBUTTONDOWN:
        setKeyValue(VK_RBUTTON, mIndex++);
        break;
    case WM_RBUTTONUP:
        setKeyValue(VK_RBUTTON, 0);
        break;

    case WM_LBUTTONDOWN:
        setKeyValue(VK_LBUTTON, mIndex++);
        break;
    case WM_LBUTTONUP:
        setKeyValue(VK_LBUTTON, 0);
        break;

    case WM_MBUTTONDOWN:
        setKeyValue(VK_MBUTTON, mIndex++);
        break;
    case WM_MBUTTONUP:
        setKeyValue(VK_MBUTTON, 0);
        break;

    case WM_XBUTTONDOWN:
        setXButtonValue(GET_XBUTTON_WPARAM(wparam), mIndex++);
        break;

    case WM_XBUTTONUP:
        setXButtonValue(GET_XBUTTON_WPARAM(wparam), 0);
        break;

    default:
        break;
    }
}

void DesktopInput::capture_cursor(bool capture) {
    mCaptureMouse = capture;
    if (mCaptureMouse) {
        centerMouse();
        mMousePosition = getWindowCenter(mWindow);
    }
}

void system::mouse::set_visible(bool visible) {
    ShowCursor(visible);
}
