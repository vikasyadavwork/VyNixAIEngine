#include <vx/Core/Window.h>

#include <vx/Platform/Windows/Win32Window.h>

namespace vx
{
std::unique_ptr<Window> Window::Create(const WindowProps& props)
{
    return std::make_unique<Win32Window>(props);
}
} // namespace vx