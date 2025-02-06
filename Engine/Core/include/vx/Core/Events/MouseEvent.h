#pragma once
#include <vx/Core/Event.h>
namespace vx
{
class MouseMovedEvent : public Event
{
  public:
    MouseMovedEvent(float x, float y) : X(x), Y(y) {}
    float X, Y;
    EventType GetEventType() const override
    {
        return EventType::MouseMoved;
    }
    std::string_view GetName() const override
    {
        return "MouseMovedEvent";
    }
};
class MouseScrolledEvent : public Event
{
  public:
    explicit MouseScrolledEvent(float delta) : Delta(delta) {}
    float Delta;
    EventType GetEventType() const override
    {
        return EventType::MouseScrolled;
    }
    std::string_view GetName() const override
    {
        return "MouseScrolledEvent";
    }
};
class MouseButtonPressedEvent : public Event
{
  public:
    explicit MouseButtonPressedEvent(int button) : Button(button) {}
    int Button;
    EventType GetEventType() const override
    {
        return EventType::MouseButtonPressed;
    }
    std::string_view GetName() const override
    {
        return "MouseButtonPressedEvent";
    }
};
class MouseButtonReleasedEvent : public Event
{
  public:
    explicit MouseButtonReleasedEvent(int button) : Button(button) {}
    int Button;
    EventType GetEventType() const override
    {
        return EventType::MouseButtonReleased;
    }
    std::string_view GetName() const override
    {
        return "MouseButtonReleasedEvent";
    }
};
} // namespace vx
