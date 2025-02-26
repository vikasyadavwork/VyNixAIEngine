#pragma once
#include <memory>
#include <vx/Core/Layer.h>
#include <vx/Core/LayerStack.h>
#include <vx/Core/Timer.h>
#include <vx/Core/Window.h>

namespace vx
{
class Application
{
  public:
    explicit Application(const WindowProps& props = {});
    virtual ~Application();

    void Run(uint64_t maxFrames = 0);
    void Close();
    Window& GetWindow() noexcept
    {
        return *m_Window;
    }
    const Window& GetWindow() const noexcept
    {
        return *m_Window;
    }

    // Layer
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

  private:
    void OnEvent(Event& event);

  private:
    bool m_Running = true;
    std::unique_ptr<Window> m_Window;
    LayerStack m_LayerStack;
    Timer m_Timer;
};
} // namespace vx
