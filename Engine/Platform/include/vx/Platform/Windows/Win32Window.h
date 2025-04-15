#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <vx/Core/Window.h>

namespace vx
{
class GraphicsContext;

class Win32Window : public Window
{
  public:
    explicit Win32Window(const WindowProps& props);
    ~Win32Window() override;

    void OnUpdate() override;
    void PollEvents() override;
    void Present() override;
    bool ShouldClose() const override;
    bool IsFocused() const override
    {
        return m_Focused;
    }
    void* GetNativeHandle() const override
    {
        return m_Handle;
    }
    void SetEventCallback(EventCallbackFn callback) override;

    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;

    const std::string& GetTitle() const override;
    void SetTitle(const std::string& title) override;

  private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void Dispatch(Event& event);
    WindowProps m_Props;
    HWND m_Handle = nullptr;
    EventCallbackFn m_EventCallback;
    bool m_ShouldClose = false;
    bool m_Focused = true;
    HDC m_DeviceContext = nullptr;

  private:
    std::unique_ptr<GraphicsContext> m_Context;
};

} // namespace vx
