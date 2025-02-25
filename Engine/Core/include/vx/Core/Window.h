#pragma once

#include <memory>
#include <string>

#include "Events/WindowEvent.h"
#include <functional>
#include <vx/Core/Types.h>

namespace vx
{

struct WindowProps
{
    std::string Title = "VyNix AI Engine";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool Visible = true;
};

class Window
{
  public:
    using EventCallbackFn = std::function<void(Event&)>;

    virtual ~Window() = default;

    virtual void OnUpdate() = 0;
    virtual void PollEvents() = 0;
    virtual void Present() = 0;

    virtual bool ShouldClose() const = 0;
    virtual bool IsFocused() const = 0;
    virtual void* GetNativeHandle() const = 0;

    virtual void SetEventCallback(EventCallbackFn callback) = 0;

    virtual uint32_t GetWidth() const = 0;

    virtual uint32_t GetHeight() const = 0;

    virtual const std::string& GetTitle() const = 0;

    virtual void SetTitle(const std::string& title) = 0;

    static std::unique_ptr<Window> Create(const WindowProps& props = {});

  protected:
};

} // namespace vx
