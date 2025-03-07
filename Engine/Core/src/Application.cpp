#include <algorithm>
#include <chrono>
#include <thread>
#include <vx/Core/Application.h>
#include <vx/Core/Input.h>
#include <vx/Core/Log.h>

namespace vx
{
Application::Application(const WindowProps& props) : m_Window(Window::Create(props))
{
    Input::Reset();
    m_Window->SetEventCallback([this](Event& event) { OnEvent(event); });
}

Application::~Application()
{
    // GPU objects owned by layers must die while the window's context is valid.
    m_Window->SetEventCallback({});
    m_LayerStack.Clear();
}

void Application::Run(uint64_t maxFrames)
{
    VX_LOG_INFO("VyNix AI Engine Started!");
    using Clock = std::chrono::steady_clock;
    auto previous = Clock::now();
    constexpr double fixedStep = 1.0 / 120.0;
    double accumulator = 0.0;
    uint64_t frames = 0;
    while (m_Running)
    {
        m_Window->PollEvents();
        if (!m_Running || m_Window->ShouldClose())
            break;

        const auto now = Clock::now();
        const double measured = std::chrono::duration<double>(now - previous).count();
        previous = now;
        accumulator += maxFrames ? 1.0 / 60.0 : std::clamp(measured, 0.0, 0.1);

        if (m_Window->GetWidth() == 0 || m_Window->GetHeight() == 0)
        {
            accumulator = 0;
            Input::EndFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        while (accumulator + 1e-12 >= fixedStep && m_Running)
        {
            for (Layer* layer : m_LayerStack)
            {
                layer->OnUpdate(static_cast<float>(fixedStep));
                if (!m_Running)
                    break;
            }
            accumulator -= fixedStep;
        }
        if (!m_Running)
            break;
        for (Layer* layer : m_LayerStack)
        {
            layer->OnRender();
            if (!m_Running)
                break;
        }
        if (!m_Running)
            break;
        m_Window->Present();
        Input::EndFrame();
        ++frames;
        if (maxFrames && frames >= maxFrames)
            Close();
        if (!maxFrames)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Application::OnEvent(Event& event)
{
    if (event.GetEventType() == EventType::WindowClose)
        Close();
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
    {
        (*it)->OnEvent(event);
        if (event.Handled)
            break;
    }
}

void Application::Close()
{
    if (m_Running)
        VX_LOG_INFO("VyNix AI Engine Closed!");
    m_Running = false;
}
void Application::PushLayer(Layer* layer)
{
    m_LayerStack.PushLayer(layer);
}
void Application::PushOverlay(Layer* overlay)
{
    m_LayerStack.PushOverlay(overlay);
}
} // namespace vx
