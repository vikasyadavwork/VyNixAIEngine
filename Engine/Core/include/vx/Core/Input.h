#pragma once

namespace vx
{
// Keys use Win32 virtual-key values. Mouse buttons: 0 left, 1 right, 2 middle.
// Edge queries are valid for one rendered frame. Handle discrete UI controls
// in OnRender; continuous movement belongs in the fixed OnUpdate callback.
class Input
{
  public:
    static bool IsKeyDown(int key) noexcept;
    static bool IsKeyPressed(int key) noexcept;
    static bool IsKeyReleased(int key) noexcept;
    static bool IsMouseButtonDown(int button) noexcept;
    static bool IsMouseButtonPressed(int button) noexcept;
    static bool IsMouseButtonReleased(int button) noexcept;
    static float MouseX() noexcept;
    static float MouseY() noexcept;
    static float ScrollDelta() noexcept;
    static bool HasFocus() noexcept;

    // Platform/application hooks. Applications normally only use the queries.
    static void SetKeyDown(int key, bool down) noexcept;
    static void SetMouseButtonDown(int button, bool down) noexcept;
    static void SetMousePosition(float x, float y) noexcept;
    static void AddScroll(float delta) noexcept;
    static void SetFocus(bool focused) noexcept;
    static void EndFrame() noexcept;
    static void Reset() noexcept;
};
} // namespace vx
