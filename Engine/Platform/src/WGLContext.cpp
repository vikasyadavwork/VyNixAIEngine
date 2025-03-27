#include "vx/Platform/Windows/WGLContext.h"
#include "vx/Core/Log.h"

#include <GL/gl.h>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace vx
{
namespace
{
template <class T> T Extension(const char* name)
{
    const auto proc = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::intptr_t>(proc);
    return (value == 0 || value == 1 || value == 2 || value == 3 || value == -1)
               ? nullptr
               : reinterpret_cast<T>(proc);
}
} // namespace

WGLContext::WGLContext(HWND windowHandle, HDC deviceContext)
    : m_WindowHandle(windowHandle), m_DeviceContext(deviceContext)
{
}

WGLContext::~WGLContext()
{
    if (m_RenderContext)
    {
        if (wglGetCurrentContext() == m_RenderContext)
            wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(m_RenderContext);
    }
}

void WGLContext::Init()
{
    if (m_RenderContext)
        return;
    if (!m_WindowHandle || !m_DeviceContext)
        throw std::runtime_error(
            "Cannot create OpenGL context without a window and device context.");

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    if (!GetPixelFormat(m_DeviceContext))
    {
        const int format = ChoosePixelFormat(m_DeviceContext, &pfd);
        if (!format || !SetPixelFormat(m_DeviceContext, format, &pfd))
            throw std::runtime_error("Windows could not configure an OpenGL pixel format.");
    }
    HGLRC legacy = wglCreateContext(m_DeviceContext);
    if (!legacy)
        throw std::runtime_error("Windows could not create an OpenGL context.");
    if (!wglMakeCurrent(m_DeviceContext, legacy))
    {
        wglDeleteContext(legacy);
        throw std::runtime_error("Windows could not activate the OpenGL context.");
    }

    // Extension entry points require a current bootstrap context. Prefer 3.3
    // core; keep the legacy context for the Windows software/RDP driver.
    using CreateContextAttribs = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
    const auto createContext = Extension<CreateContextAttribs>("wglCreateContextAttribsARB");
    if (createContext)
    {
        constexpr int attributes[] = {
            0x2091, 3, // WGL_CONTEXT_MAJOR_VERSION_ARB
            0x2092, 3, // WGL_CONTEXT_MINOR_VERSION_ARB
            0x9126, 1, // WGL_CONTEXT_PROFILE_MASK_ARB: CORE_PROFILE_BIT_ARB
            0};
        if (HGLRC modern = createContext(m_DeviceContext, nullptr, attributes))
        {
            if (wglMakeCurrent(m_DeviceContext, modern))
            {
                wglDeleteContext(legacy);
                legacy = modern;
            }
            else
            {
                wglDeleteContext(modern);
                if (!wglMakeCurrent(m_DeviceContext, legacy))
                {
                    wglDeleteContext(legacy);
                    throw std::runtime_error("OpenGL context activation failed.");
                }
            }
        }
    }
    m_RenderContext = legacy;
    const auto* version = glGetString(GL_VERSION);
    const auto* renderer = glGetString(GL_RENDERER);
    VX_LOG_INFO(std::string("OpenGL ") +
                (version ? reinterpret_cast<const char*>(version) : "unknown") + " | " +
                (renderer ? reinterpret_cast<const char*>(renderer) : "unknown renderer"));
    SetVSync(true);
}

void WGLContext::SetVSync(bool enabled)
{
    using SwapInterval = BOOL(WINAPI*)(int);
    if (const auto setInterval = Extension<SwapInterval>("wglSwapIntervalEXT"))
        setInterval(enabled ? 1 : 0);
}

void WGLContext::SwapBuffers()
{
    ::SwapBuffers(m_DeviceContext);
}
} // namespace vx
