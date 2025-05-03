#include <Windows.h>
#include <gl/GL.h>
#include <iostream>
#include <stdexcept>
#include <vx/Core/Application.h>
#include <vx/Core/Input.h>
#include <vx/Platform/Windows/Win32Window.h>

namespace
{
void Require(bool value, const char* reason)
{
    if (!value)
        throw std::runtime_error(reason);
}
struct Counts
{
    int updates = 0, renders = 0, presses = 0, resizes = 0, closes = 0;
    bool detachedWithContext = false;
};
class Probe final : public vx::Layer
{
  public:
    Probe(vx::Application& app, Counts& counts, bool earlyClose = false)
        : app(app), counts(counts), earlyClose(earlyClose)
    {
    }
    void OnUpdate(float dt) override
    {
        Require(dt > .008f && dt < .009f, "Update is not a 120 Hz fixed step");
        ++counts.updates;
        if (earlyClose)
            app.Close();
    }
    void OnRender() override
    {
        ++counts.renders;
        if (vx::Input::IsKeyPressed('W'))
            ++counts.presses;
        glClearColor(.01f, .02f, .03f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void OnEvent(Event& event) override
    {
        if (event.GetEventType() == EventType::WindowResize)
            ++counts.resizes;
        if (event.GetEventType() == EventType::WindowClose)
            ++counts.closes;
    }
    void OnDetach() override
    {
        counts.detachedWithContext = wglGetCurrentContext() != nullptr;
    }

  private:
    vx::Application& app;
    Counts& counts;
    bool earlyClose;
};
} // namespace

int main()
{
    try
    {
        Counts counts;
        {
            vx::Application app({"Hidden platform verification", 640, 360, false});
            auto& window = app.GetWindow();
            Require(window.GetWidth() == 640 && window.GetHeight() == 360,
                    "Client size differs from WindowProps");
            Require(!IsWindowVisible(static_cast<HWND>(window.GetNativeHandle())),
                    "Smoke window is visible");
            app.PushLayer(new Probe(app, counts));
            HWND handle = static_cast<HWND>(window.GetNativeHandle());
            RECT bounds{0, 0, 800, 450};
            AdjustWindowRectEx(&bounds, WS_OVERLAPPEDWINDOW, FALSE, 0);
            SetWindowPos(handle, nullptr, 0, 0, bounds.right - bounds.left,
                         bounds.bottom - bounds.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            Require(window.GetWidth() == 800 && window.GetHeight() == 450,
                    "Resize did not update client size");
            SendMessageW(handle, WM_KEYDOWN, 'W', 0);
            SendMessageW(handle, WM_KILLFOCUS, 0, 0);
            Require(!vx::Input::IsKeyDown('W') && !window.IsFocused(),
                    "Focus loss did not clear input");
            SendMessageW(handle, WM_SETFOCUS, 0, 0);
            SendMessageW(handle, WM_KEYDOWN, 'W', 0);
            SendMessageW(handle, WM_MOUSEMOVE, 0, MAKELPARAM(80, 90));
            Require(vx::Input::MouseX() == 80 && vx::Input::MouseY() == 90,
                    "Mouse event was not recorded");
            app.Run(3);
            Require(counts.updates == 6 && counts.renders == 3,
                    "Bounded fixed-step loop had wrong counts");
            Require(counts.presses == 1, "Key edge did not last exactly one rendered frame");
            Require(counts.resizes > 0, "Resize event did not reach layer");
        }
        Require(counts.detachedWithContext, "Layer detached after context destruction");
        Counts early;
        {
            vx::Application app({"Early close verification", 320, 200, false});
            app.PushLayer(new Probe(app, early, true));
            app.Run(4);
        }
        Require(early.updates == 1 && early.renders == 0 && early.detachedWithContext,
                "Close during update did not stop rendering safely");
        Counts closeMessage;
        {
            vx::Application app({"Close event verification", 320, 200, false});
            app.PushLayer(new Probe(app, closeMessage));
            SendMessageW(static_cast<HWND>(app.GetWindow().GetNativeHandle()), WM_CLOSE, 0, 0);
            Require(wglGetCurrentContext() != nullptr,
                    "WM_CLOSE destroyed context before layer cleanup");
            app.Run(2);
        }
        Require(closeMessage.closes == 1 && closeMessage.renders == 0 &&
                    closeMessage.detachedWithContext,
                "Window close event did not preserve orderly shutdown");
        std::cout << "Hidden OpenGL window, fixed loop, input dispatch, resize, and context "
                     "lifetime checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
