#include <vx/Core/Application.h>
#include <vx/Core/Log.h>
#include <Windows.h>

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
while (m_Running) {
    ProcessMessages();
}
}

void Application::Close()
{
    VX_LOG_INFO("VyNix AI Engine Closed!");
    m_Running = false;
}

void Application::ProcessMessages()
{
    MSG msg{};

    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) {
            m_Running = false;
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

} // namespace vx