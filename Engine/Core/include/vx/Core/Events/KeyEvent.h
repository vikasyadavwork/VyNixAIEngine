#pragma once
#include <vx/Core/Event.h>
namespace vx
{
class KeyPressedEvent : public Event
{
  public:
    KeyPressedEvent(int key, bool repeated = false) : KeyCode(key), Repeated(repeated) {}
    int KeyCode;
    bool Repeated;
    int GetKeyCode() const noexcept
    {
        return KeyCode;
    }
    bool IsRepeat() const noexcept
    {
        return Repeated;
    }
    EventType GetEventType() const override
    {
        return EventType::KeyPressed;
    }
    std::string_view GetName() const override
    {
        return "KeyPressedEvent";
    }
};
class KeyReleasedEvent : public Event
{
  public:
    explicit KeyReleasedEvent(int key) : KeyCode(key) {}
    int KeyCode;
    int GetKeyCode() const noexcept
    {
        return KeyCode;
    }
    EventType GetEventType() const override
    {
        return EventType::KeyReleased;
    }
    std::string_view GetName() const override
    {
        return "KeyReleasedEvent";
    }
};
} // namespace vx
