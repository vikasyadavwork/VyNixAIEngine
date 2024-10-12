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
    private:
        std::unique_ptr<Window> m_Window;
    };
}