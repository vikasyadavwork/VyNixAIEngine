#pragma once

#include <memory>
#include <string>

#include <vx/Core/Types.h>

namespace vx
{

    struct WindowProps
    {
        std::string Title = "VyNix AI Engine";
        uint32_t Width = 1280;
        uint32_t Height = 720;
    };

    class Window
    {
    public:

        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;

        virtual uint32_t GetHeight() const = 0;

        virtual const std::string& GetTitle() const = 0;

        virtual void SetTitle(const std::string& title) = 0;

        static std::unique_ptr<Window> Create(const WindowProps& props = {});
    };

}