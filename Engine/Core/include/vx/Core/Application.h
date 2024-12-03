#pragma once
#include <memory>
#include <vx/Core/Window.h>

namespace vx
{
class Application
{
public:
    Application();
    virtual ~Application();

    void Run();
    void Close();
private:
    void ProcessMessages();
private:
    bool m_Running = true;
    std::unique_ptr<Window> m_Window;
};
}