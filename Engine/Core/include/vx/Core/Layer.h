#pragma once

#include <string>

#include "vx/Core/Event.h"

namespace vx
{

class Layer
{
  public:
    explicit Layer(std::string name = "Layer");
    virtual ~Layer() = default;

    // Lifetime
    virtual void OnAttach() {}
    virtual void OnDetach() {}

    // Per Frame
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}

    // Events
    virtual void OnEvent(Event& event) {}

    [[nodiscard]] const std::string& GetName() const noexcept
    {
        return m_Name;
    }

  protected:
    std::string m_Name;
};

} // namespace vx