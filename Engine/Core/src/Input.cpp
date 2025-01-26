#include <array>
#include <vx/Core/Input.h>

namespace
{
std::array<bool, 256> keys{}, pressed{}, released{};
std::array<bool, 5> buttons{}, buttonPressed{}, buttonReleased{};
float mouseX = 0, mouseY = 0, scroll = 0;
bool focused = true;
bool ValidKey(int key) noexcept
{
    return key >= 0 && key < 256;
}
bool ValidButton(int button) noexcept
{
    return button >= 0 && button < 5;
}
} // namespace

namespace vx
{
bool Input::IsKeyDown(int key) noexcept
{
    return ValidKey(key) && keys[key];
}
bool Input::IsKeyPressed(int key) noexcept
{
    return ValidKey(key) && pressed[key];
}
bool Input::IsKeyReleased(int key) noexcept
{
    return ValidKey(key) && released[key];
}
bool Input::IsMouseButtonDown(int button) noexcept
{
    return ValidButton(button) && buttons[button];
}
bool Input::IsMouseButtonPressed(int button) noexcept
{
    return ValidButton(button) && buttonPressed[button];
}
bool Input::IsMouseButtonReleased(int button) noexcept
{
    return ValidButton(button) && buttonReleased[button];
}
float Input::MouseX() noexcept
{
    return mouseX;
}
float Input::MouseY() noexcept
{
    return mouseY;
}
float Input::ScrollDelta() noexcept
{
    return scroll;
}
bool Input::HasFocus() noexcept
{
    return focused;
}
void Input::SetKeyDown(int key, bool down) noexcept
{
    if (!ValidKey(key))
        return;
    if (down && !keys[key])
        pressed[key] = true;
    if (!down && keys[key])
        released[key] = true;
    keys[key] = down;
}
void Input::SetMouseButtonDown(int button, bool down) noexcept
{
    if (!ValidButton(button))
        return;
    if (down && !buttons[button])
        buttonPressed[button] = true;
    if (!down && buttons[button])
        buttonReleased[button] = true;
    buttons[button] = down;
}
void Input::SetMousePosition(float x, float y) noexcept
{
    mouseX = x;
    mouseY = y;
}
void Input::AddScroll(float delta) noexcept
{
    scroll += delta;
}
void Input::SetFocus(bool value) noexcept
{
    focused = value;
    if (!focused)
        Reset();
}
void Input::EndFrame() noexcept
{
    pressed.fill(false);
    released.fill(false);
    buttonPressed.fill(false);
    buttonReleased.fill(false);
    scroll = 0;
}
void Input::Reset() noexcept
{
    keys.fill(false);
    buttons.fill(false);
    EndFrame();
}
} // namespace vx
