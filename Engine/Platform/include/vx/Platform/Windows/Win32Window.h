#pragma once

#include <Windows.h>

#include <vx/Core/Window.h>

namespace vx
{

    class Win32Window : public Window
    {
    public:
        explicit Win32Window(const WindowProps& props);
        ~Win32Window() override;

        void OnUpdate() override;

        uint32_t GetWidth() const override;
        uint32_t GetHeight() const override;

        const std::string& GetTitle() const override;
        void SetTitle(const std::string& title) override;

    private:
        WindowProps m_Props;
        HWND m_Handle = nullptr;
    };

}