#pragma once
#include <string_view>

enum class EventType
{
    None = 0,

    // Window
    WindowClose,
    WindowResize,

    // Keyboard
    KeyPressed,
    KeyReleased,

    // Mouse
    MouseMoved,
    MouseScrolled,
    MouseButtonPressed,
    MouseButtonReleased
};

class Event
{
public:
    virtual ~Event() = default;

    virtual EventType GetEventType() const = 0;

    virtual std::string_view GetName() const = 0;
};