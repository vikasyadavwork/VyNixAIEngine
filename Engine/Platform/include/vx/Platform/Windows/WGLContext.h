#pragma once

#include "vx/Renderer/GraphicsContext.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace vx
{

class WGLContext : public GraphicsContext
{
  public:
    WGLContext(HWND windowHandle, HDC deviceContext);
    ~WGLContext() override;

    void Init() override;
    void SwapBuffers() override;
    void SetVSync(bool enabled);

  private:
    HWND m_WindowHandle;
    HDC m_DeviceContext;
    HGLRC m_RenderContext = nullptr;
};

} // namespace vx
