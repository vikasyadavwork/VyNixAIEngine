#include <vx/Core/Application.h>
#include <vx/Core/Log.h>

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
    }

}