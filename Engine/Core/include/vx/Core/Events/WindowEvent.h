#pragma once
#include <vx/Core/Event.h>

namespace vx
{
class WindowEvent : public Event
{
  public:
    virtual ~WindowEvent() = default;
};

class WindowCloseEvent : public WindowEvent
{
  public:
    EventType GetEventType() const override
    {
        return EventType::WindowClose;
    }
    std::string_view GetName() const override
    {
        return "WindowCloseEvent";
    }
};

class WindowResizeEvent : public WindowEvent
{
  public:
    WindowResizeEvent(unsigned width, unsigned height) : Width(width), Height(height) {}
    unsigned Width, Height;
    unsigned GetWidth() const noexcept
    {
        return Width;
    }
    unsigned GetHeight() const noexcept
    {
        return Height;
    }
    EventType GetEventType() const override
    {
        return EventType::WindowResize;
    }
    std::string_view GetName() const override
    {
        return "WindowResizeEvent";
    }
};
class WindowFocusEvent : public WindowEvent
{
  public:
    explicit WindowFocusEvent(bool focused) : Focused(focused) {}
    bool Focused;
    bool IsFocused() const noexcept
    {
        return Focused;
    }
    EventType GetEventType() const override
    {
        return EventType::WindowFocus;
    }
    std::string_view GetName() const override
    {
        return "WindowFocusEvent";
    }
};
} // namespace vx
