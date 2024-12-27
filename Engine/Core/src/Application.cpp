#include <vx/Core/Application.h>
#include <vx/Core/Log.h>
#include <vx/Core/Events/WindowEvent.h>

namespace vx
{

Application::Application()
{
    m_Window = Window::Create();
}

Application::~Application() = default;

void Application::Run()
{
    VX_LOG_INFO("VyNix AI Engine Started!");

    WindowCloseEvent event;
    VX_LOG_INFO(event.GetName());

    while (m_Running) {
        m_Window->OnUpdate();

        if (m_Window->ShouldClose()) {
            Close();
        }
    }
}

void Application::Close()
{
    VX_LOG_INFO("VyNix AI Engine Closed!");
    m_Running = false;
}

void Application::ProcessMessages()
{

}

} // namespace vx