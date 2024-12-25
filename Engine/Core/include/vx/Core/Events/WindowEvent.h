#pragma once
#include <vx/Core/Event.h>

namespace vx {
class WindowEvent : public Event
{
public:
    virtual ~WindowEvent() = default;
};

class WindowCloseEvent : public WindowEvent {
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
}