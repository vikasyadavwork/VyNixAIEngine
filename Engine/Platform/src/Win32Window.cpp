#include <vx/Platform/Windows/Win32Window.h>
#include <vx/Core/Log.h>

namespace vx
{

static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

Win32Window::Win32Window(const WindowProps& props)
    : m_Props(props)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);

    wc.style = CS_HREDRAW | CS_VREDRAW;

    wc.lpfnWndProc = WindowProc;

    wc.hInstance = GetModuleHandleW(nullptr);

    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    wc.lpszClassName = L"VyNixWindowClass";

    if (!RegisterClassExW(&wc))
    {
        VX_LOG_FATAL("Failed to register window class.");
        return;
    }

    m_Handle = CreateWindowExW(
        0,
        L"VyNixWindowClass",
        L"VyNix AI Engine",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        static_cast<int>(m_Props.Width),
        static_cast<int>(m_Props.Height),
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (!m_Handle)
    {
        VX_LOG_FATAL("Failed to create Win32 window.");
        return;
    }

    ShowWindow(m_Handle, SW_SHOW);
    UpdateWindow(m_Handle);

    VX_LOG_INFO("Window Class Registered");
}

Win32Window::~Win32Window()
{
    VX_LOG_INFO("Destroying Win32 Window");
}

void Win32Window::OnUpdate()
{
}

uint32_t Win32Window::GetWidth() const
{
    return m_Props.Width;
}

uint32_t Win32Window::GetHeight() const
{
    return m_Props.Height;
}

const std::string& Win32Window::GetTitle() const
{
    return m_Props.Title;
}

void Win32Window::SetTitle(const std::string& title)
{
    m_Props.Title = title;
}

}