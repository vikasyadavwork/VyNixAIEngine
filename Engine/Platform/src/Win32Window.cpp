#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vx/Core/Events/KeyEvent.h>
#include <vx/Core/Events/MouseEvent.h>
#include <vx/Core/Input.h>
#include <vx/Core/Log.h>
#include <vx/Platform/Windows/WGLContext.h>
#include <vx/Platform/Windows/Win32Window.h>
#include <vx/Renderer/GraphicsContext.h>
#include <windowsx.h>

namespace
{
RECT WorkArea(HWND window = nullptr)
{
    MONITORINFO monitor{};
    monitor.cbSize = sizeof(monitor);
    if (window && GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
        return monitor.rcWork;
    RECT area{};
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0))
        area = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    return area;
}

std::wstring Wide(const std::string& text)
{
    if (text.empty())
        return {};
    const int length =
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(),
                        length);
    return result;
}
} // namespace

namespace vx
{
LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        window = static_cast<Win32Window*>(create->lpCreateParams);
        window->m_Handle = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    if (window)
        return window->HandleMessage(message, wParam, lParam);
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

Win32Window::Win32Window(const WindowProps& props) : m_Props(props), m_Focused(props.Visible)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"VyNixWindowClass";
    if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("Failed to register the VyNix window class.");

    RECT bounds{0, 0, static_cast<LONG>(props.Width), static_cast<LONG>(props.Height)};
    constexpr DWORD style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRectEx(&bounds, style, FALSE, 0);
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    if (props.Visible && props.Width && props.Height)
    {
        const RECT area = WorkArea();
        const LONG frameWidth = bounds.right - bounds.left - static_cast<LONG>(props.Width);
        const LONG frameHeight = bounds.bottom - bounds.top - static_cast<LONG>(props.Height);
        // Preserve the client aspect while leaving 16 pixels around the frame.
        const LONG availableWidth = std::max(1L, area.right - area.left - frameWidth - 32);
        const LONG availableHeight = std::max(1L, area.bottom - area.top - frameHeight - 32);
        const double scale = std::min({1.0, static_cast<double>(availableWidth) / props.Width,
                                       static_cast<double>(availableHeight) / props.Height});
        if (scale < 1.0)
        {
            bounds = {0, 0, std::max(1L, static_cast<LONG>(props.Width * scale)),
                      std::max(1L, static_cast<LONG>(props.Height * scale))};
            AdjustWindowRectEx(&bounds, style, FALSE, 0);
            x = area.left + (area.right - area.left - (bounds.right - bounds.left)) / 2;
            y = area.top + (area.bottom - area.top - (bounds.bottom - bounds.top)) / 2;
        }
    }
    const auto title = Wide(props.Title);
    m_Handle =
        CreateWindowExW(0, wc.lpszClassName, title.c_str(), style, x, y, bounds.right - bounds.left,
                        bounds.bottom - bounds.top, nullptr, nullptr, wc.hInstance, this);
    if (!m_Handle)
        throw std::runtime_error("Failed to create the VyNix window.");
    try
    {
        m_DeviceContext = GetDC(m_Handle);
        if (!m_DeviceContext)
            throw std::runtime_error("Failed to get the window device context.");
        m_Context = std::make_unique<WGLContext>(m_Handle, m_DeviceContext);
        m_Context->Init();
        RECT client{};
        GetClientRect(m_Handle, &client);
        m_Props.Width = static_cast<uint32_t>(client.right);
        m_Props.Height = static_cast<uint32_t>(client.bottom);
        if (props.Visible)
        {
            ShowWindow(m_Handle, SW_SHOW);
            UpdateWindow(m_Handle);
        }
        m_Focused = GetFocus() == m_Handle;
        Input::SetFocus(m_Focused);
    }
    catch (...)
    {
        m_Context.reset();
        if (m_DeviceContext)
            ReleaseDC(m_Handle, m_DeviceContext);
        SetWindowLongPtrW(m_Handle, GWLP_USERDATA, 0);
        DestroyWindow(m_Handle);
        m_Handle = nullptr;
        throw;
    }
}

Win32Window::~Win32Window()
{
    m_EventCallback = {};
    m_Context.reset();
    if (m_DeviceContext && m_Handle)
        ReleaseDC(m_Handle, m_DeviceContext);
    if (m_Handle)
    {
        SetWindowLongPtrW(m_Handle, GWLP_USERDATA, 0);
        DestroyWindow(m_Handle);
    }
}

void Win32Window::Dispatch(Event& event)
{
    if (m_EventCallback)
        m_EventCallback(event);
}

LRESULT Win32Window::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_GETMINMAXINFO:
    {
        if (!m_Props.Visible)
            break;
        const RECT area = WorkArea(m_Handle);
        RECT minimum{0, 0, 960, 600};
        AdjustWindowRectEx(&minimum, WS_OVERLAPPEDWINDOW, FALSE, 0);
        const LONG frameWidth = minimum.right - minimum.left - 960;
        const LONG frameHeight = minimum.bottom - minimum.top - 600;
        const LONG availableWidth = std::max(1L, area.right - area.left - frameWidth - 32);
        const LONG availableHeight = std::max(1L, area.bottom - area.top - frameHeight - 32);
        const double scale = std::min({1.0, availableWidth / 960.0, availableHeight / 600.0});
        auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
        limits->ptMinTrackSize.x = static_cast<LONG>(960 * scale) + frameWidth;
        limits->ptMinTrackSize.y = static_cast<LONG>(600 * scale) + frameHeight;
        return 0;
    }
    case WM_CLOSE:
    {
        // Keep the HWND/DC/context alive until application-owned layers detach.
        m_ShouldClose = true;
        WindowCloseEvent event;
        Dispatch(event);
        return 0;
    }
    case WM_DESTROY:
        m_ShouldClose = true;
        return 0;
    case WM_SIZE:
    {
        m_Props.Width = LOWORD(lParam);
        m_Props.Height = HIWORD(lParam);
        WindowResizeEvent event(m_Props.Width, m_Props.Height);
        Dispatch(event);
        return 0;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        m_Focused = message == WM_SETFOCUS;
        Input::SetFocus(m_Focused);
        if (!m_Focused && GetCapture() == m_Handle)
            ReleaseCapture();
        WindowFocusEvent event(m_Focused);
        Dispatch(event);
        return 0;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const int key = static_cast<int>(wParam);
        Input::SetKeyDown(key, true);
        KeyPressedEvent event(key, (lParam & (1LL << 30)) != 0);
        Dispatch(event);
        if (message == WM_SYSKEYDOWN)
            return DefWindowProcW(m_Handle, message, wParam, lParam);
        return 0;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const int key = static_cast<int>(wParam);
        Input::SetKeyDown(key, false);
        KeyReleasedEvent event(key);
        Dispatch(event);
        if (message == WM_SYSKEYUP)
            return DefWindowProcW(m_Handle, message, wParam, lParam);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        const float x = static_cast<float>(GET_X_LPARAM(lParam));
        const float y = static_cast<float>(GET_Y_LPARAM(lParam));
        Input::SetMousePosition(x, y);
        MouseMovedEvent event(x, y);
        Dispatch(event);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    {
        const int button = (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP)   ? 0
                           : (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP) ? 1
                                                                                    : 2;
        const bool down =
            message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN;
        Input::SetMousePosition(static_cast<float>(GET_X_LPARAM(lParam)),
                                static_cast<float>(GET_Y_LPARAM(lParam)));
        Input::SetMouseButtonDown(button, down);
        if (down)
        {
            SetCapture(m_Handle);
            MouseButtonPressedEvent event(button);
            Dispatch(event);
        }
        else
        {
            if (!Input::IsMouseButtonDown(0) && !Input::IsMouseButtonDown(1) &&
                !Input::IsMouseButtonDown(2))
                ReleaseCapture();
            MouseButtonReleasedEvent event(button);
            Dispatch(event);
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        const float delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        Input::AddScroll(delta);
        MouseScrolledEvent event(delta);
        Dispatch(event);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcW(m_Handle, message, wParam, lParam);
}

void Win32Window::OnUpdate()
{
    PollEvents();
}
void Win32Window::PollEvents()
{
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            m_ShouldClose = true;
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
}
void Win32Window::Present()
{
    if (m_Context && !m_ShouldClose)
        m_Context->SwapBuffers();
}
bool Win32Window::ShouldClose() const
{
    return m_ShouldClose;
}
void Win32Window::SetEventCallback(EventCallbackFn callback)
{
    m_EventCallback = std::move(callback);
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
    const auto wide = Wide(title);
    SetWindowTextW(m_Handle, wide.c_str());
}
} // namespace vx
