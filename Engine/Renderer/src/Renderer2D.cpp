#include "vx/Renderer/Renderer2D.h"
#include "vx/Core/Log.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <GL/gl.h>
#include <wincodec.h>
#include <wrl/client.h>

#ifdef DrawText
#undef DrawText
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <utility>
#include <vector>

namespace vx
{
namespace
{
constexpr GLenum ArrayBuffer = 0x8892, StreamDraw = 0x88E0;
constexpr GLenum VertexShader = 0x8B31, FragmentShader = 0x8B30;
constexpr GLenum CompileStatus = 0x8B81, LinkStatus = 0x8B82;
constexpr GLenum Texture0 = 0x84C0, ClampToEdge = 0x812F;
constexpr std::size_t MaxVertices = 6000;
constexpr int FontWidth = 768, FontHeight = 384;
constexpr int FontCellWidth = 48, FontCellHeight = 64, FontPixels = 36, FontMargin = 4;

struct Vertex
{
    float X, Y, U, V, R, G, B, A;
};

template <class T> T LoadGL(const char* name)
{
    auto proc = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::intptr_t>(proc);
    if (value == 0 || value == 1 || value == 2 || value == 3 || value == -1)
    {
        const auto module = GetModuleHandleW(L"opengl32.dll");
        proc = module ? GetProcAddress(module, name) : nullptr;
    }
    return reinterpret_cast<T>(proc);
}

struct GLFunctions
{
    void(APIENTRY* GenVertexArrays)(GLsizei, GLuint*) = nullptr;
    void(APIENTRY* BindVertexArray)(GLuint) = nullptr;
    void(APIENTRY* DeleteVertexArrays)(GLsizei, const GLuint*) = nullptr;
    void(APIENTRY* GenBuffers)(GLsizei, GLuint*) = nullptr;
    void(APIENTRY* BindBuffer)(GLenum, GLuint) = nullptr;
    void(APIENTRY* BufferData)(GLenum, std::ptrdiff_t, const void*, GLenum) = nullptr;
    void(APIENTRY* DeleteBuffers)(GLsizei, const GLuint*) = nullptr;
    GLuint(APIENTRY* CreateShader)(GLenum) = nullptr;
    void(APIENTRY* ShaderSource)(GLuint, GLsizei, const char* const*, const GLint*) = nullptr;
    void(APIENTRY* CompileShader)(GLuint) = nullptr;
    void(APIENTRY* GetShaderiv)(GLuint, GLenum, GLint*) = nullptr;
    void(APIENTRY* GetShaderInfoLog)(GLuint, GLsizei, GLsizei*, char*) = nullptr;
    void(APIENTRY* DeleteShader)(GLuint) = nullptr;
    GLuint(APIENTRY* CreateProgram)() = nullptr;
    void(APIENTRY* AttachShader)(GLuint, GLuint) = nullptr;
    void(APIENTRY* LinkProgram)(GLuint) = nullptr;
    void(APIENTRY* GetProgramiv)(GLuint, GLenum, GLint*) = nullptr;
    void(APIENTRY* GetProgramInfoLog)(GLuint, GLsizei, GLsizei*, char*) = nullptr;
    void(APIENTRY* UseProgram)(GLuint) = nullptr;
    void(APIENTRY* DeleteProgram)(GLuint) = nullptr;
    GLint(APIENTRY* GetUniformLocation)(GLuint, const char*) = nullptr;
    void(APIENTRY* Uniform1i)(GLint, GLint) = nullptr;
    void(APIENTRY* UniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*) = nullptr;
    void(APIENTRY* EnableVertexAttribArray)(GLuint) = nullptr;
    void(APIENTRY* VertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei,
                                        const void*) = nullptr;
    void(APIENTRY* ActiveTexture)(GLenum) = nullptr;

    bool Load()
    {
#define VX_LOAD_GL(name)                                                                           \
    name = LoadGL<decltype(name)>("gl" #name);                                                     \
    if (!name)                                                                                     \
    return false
        VX_LOAD_GL(GenVertexArrays);
        VX_LOAD_GL(BindVertexArray);
        VX_LOAD_GL(DeleteVertexArrays);
        VX_LOAD_GL(GenBuffers);
        VX_LOAD_GL(BindBuffer);
        VX_LOAD_GL(BufferData);
        VX_LOAD_GL(DeleteBuffers);
        VX_LOAD_GL(CreateShader);
        VX_LOAD_GL(ShaderSource);
        VX_LOAD_GL(CompileShader);
        VX_LOAD_GL(GetShaderiv);
        VX_LOAD_GL(GetShaderInfoLog);
        VX_LOAD_GL(DeleteShader);
        VX_LOAD_GL(CreateProgram);
        VX_LOAD_GL(AttachShader);
        VX_LOAD_GL(LinkProgram);
        VX_LOAD_GL(GetProgramiv);
        VX_LOAD_GL(GetProgramInfoLog);
        VX_LOAD_GL(UseProgram);
        VX_LOAD_GL(DeleteProgram);
        VX_LOAD_GL(GetUniformLocation);
        VX_LOAD_GL(Uniform1i);
        VX_LOAD_GL(UniformMatrix4fv);
        VX_LOAD_GL(EnableVertexAttribArray);
        VX_LOAD_GL(VertexAttribPointer);
        VX_LOAD_GL(ActiveTexture);
#undef VX_LOAD_GL
        return true;
    }
};

int PowerOfTwo(int value)
{
    int result = 1;
    while (result < value && result < 32768)
        result *= 2;
    return result;
}
} // namespace

void Camera2D::SetViewport(float width, float height)
{
    m_Width = std::max(width, 1.0f);
    m_Height = std::max(height, 1.0f);
}

void Camera2D::SetZoom(float zoom)
{
    m_Zoom = std::max(zoom, 0.001f);
}

std::array<float, 16> Camera2D::GetProjection() const
{
    const float sx = 2.0f * m_Zoom / m_Width, sy = -2.0f * m_Zoom / m_Height;
    return {sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, -1, 0, -1 - m_X * sx, 1 - m_Y * sy, 0, 1};
}

Texture2D::Texture2D(std::uint32_t handle, int width, int height, void* context)
    : m_Handle(handle), m_Width(width), m_Height(height), m_Context(context)
{
}
Texture2D::~Texture2D()
{
    Release();
}
Texture2D::Texture2D(Texture2D&& other) noexcept
{
    *this = std::move(other);
}
Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other)
    {
        Release();
        m_Handle = std::exchange(other.m_Handle, 0);
        m_Width = std::exchange(other.m_Width, 0);
        m_Height = std::exchange(other.m_Height, 0);
        m_Context = std::exchange(other.m_Context, nullptr);
    }
    return *this;
}
void Texture2D::Release()
{
    if (m_Handle && wglGetCurrentContext() == m_Context)
        glDeleteTextures(1, &m_Handle);
    m_Handle = 0;
}

struct Renderer2D::Impl
{
    GLFunctions GL;
    bool Initialized = false, Modern = false;
    HGLRC Context = nullptr;
    GLuint Program = 0, VAO = 0, VBO = 0, BatchTexture = 0;
    GLint ProjectionLocation = -1;
    int FramebufferWidth = 0, FramebufferHeight = 0;
    float LogicalWidth = 1280.0f, LogicalHeight = 720.0f;
    Camera2D Camera;
    Renderer2DStats Stats;
    std::string Backend, Error;
    std::vector<Vertex> Vertices;
    std::shared_ptr<Texture2D> White, Font;
    std::vector<std::weak_ptr<Texture2D>> Textures;
    std::array<float, 96> GlyphAdvance{};

    void Fail(std::string message)
    {
        Error = std::move(message);
        VX_LOG_ERROR(Error);
    }

    GLuint Compile(GLenum type, const char* source)
    {
        const GLuint shader = GL.CreateShader(type);
        GL.ShaderSource(shader, 1, &source, nullptr);
        GL.CompileShader(shader);
        GLint success = 0;
        GL.GetShaderiv(shader, CompileStatus, &success);
        if (!success)
        {
            std::array<char, 2048> log{};
            GL.GetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
            Fail(std::string("Renderer shader compilation failed: ") + log.data());
            GL.DeleteShader(shader);
            return 0;
        }
        return shader;
    }

    bool InitializeModern()
    {
        constexpr const char* vertexSource = R"GLSL(#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
uniform mat4 uProjection;
out vec2 vUV;
out vec4 vColor;
void main() { gl_Position = uProjection * vec4(aPosition, 0.0, 1.0); vUV = aUV; vColor = aColor; }
)GLSL";
        constexpr const char* fragmentSource = R"GLSL(#version 330 core
in vec2 vUV;
in vec4 vColor;
uniform sampler2D uTexture;
out vec4 outColor;
void main() { outColor = texture(uTexture, vUV) * vColor; }
)GLSL";
        const GLuint vertex = Compile(VertexShader, vertexSource);
        if (!vertex)
            return false;
        const GLuint fragment = Compile(FragmentShader, fragmentSource);
        if (!fragment)
        {
            GL.DeleteShader(vertex);
            return false;
        }
        Program = GL.CreateProgram();
        GL.AttachShader(Program, vertex);
        GL.AttachShader(Program, fragment);
        GL.LinkProgram(Program);
        GL.DeleteShader(vertex);
        GL.DeleteShader(fragment);
        GLint success = 0;
        GL.GetProgramiv(Program, LinkStatus, &success);
        if (!success)
        {
            std::array<char, 2048> log{};
            GL.GetProgramInfoLog(Program, static_cast<GLsizei>(log.size()), nullptr, log.data());
            Fail(std::string("Renderer shader link failed: ") + log.data());
            return false;
        }
        GL.UseProgram(Program);
        ProjectionLocation = GL.GetUniformLocation(Program, "uProjection");
        GL.Uniform1i(GL.GetUniformLocation(Program, "uTexture"), 0);
        GL.GenVertexArrays(1, &VAO);
        GL.GenBuffers(1, &VBO);
        GL.BindVertexArray(VAO);
        GL.BindBuffer(ArrayBuffer, VBO);
        GL.BufferData(ArrayBuffer, static_cast<std::ptrdiff_t>(MaxVertices * sizeof(Vertex)),
                      nullptr, StreamDraw);
        GL.EnableVertexAttribArray(0);
        GL.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<const void*>(offsetof(Vertex, X)));
        GL.EnableVertexAttribArray(1);
        GL.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<const void*>(offsetof(Vertex, U)));
        GL.EnableVertexAttribArray(2);
        GL.VertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                               reinterpret_cast<const void*>(offsetof(Vertex, R)));
        return true;
    }

    void Flush()
    {
        if (!Initialized || Vertices.empty())
            return;
        const auto projection = Camera.GetProjection();
        if (Modern)
        {
            GL.UseProgram(Program);
            GL.BindVertexArray(VAO);
            GL.BindBuffer(ArrayBuffer, VBO);
            GL.BufferData(ArrayBuffer,
                          static_cast<std::ptrdiff_t>(Vertices.size() * sizeof(Vertex)),
                          Vertices.data(), StreamDraw);
            GL.UniformMatrix4fv(ProjectionLocation, 1, GL_FALSE, projection.data());
            GL.ActiveTexture(Texture0);
        }
        else
        {
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf(projection.data());
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glEnable(GL_TEXTURE_2D);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);
            glVertexPointer(2, GL_FLOAT, sizeof(Vertex), &Vertices.front().X);
            glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), &Vertices.front().U);
            glColorPointer(4, GL_FLOAT, sizeof(Vertex), &Vertices.front().R);
        }
        glBindTexture(GL_TEXTURE_2D, BatchTexture);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(Vertices.size()));
        ++Stats.DrawCalls;
        ++Stats.TextureBinds;
        Stats.VertexCount += static_cast<std::uint32_t>(Vertices.size());
        Vertices.clear();
    }

    void Quad(float x, float y, float width, float height, Color color, GLuint texture, UVRect uv,
              float rotation)
    {
        if (!Initialized || width <= 0 || height <= 0 || color.A <= 0)
            return;
        if (BatchTexture != texture || Vertices.size() + 6 > MaxVertices)
            Flush();
        BatchTexture = texture;
        const float sine = std::sin(rotation), cosine = std::cos(rotation);
        const float halfWidth = width * 0.5f, halfHeight = height * 0.5f;
        const std::array<std::array<float, 4>, 4> corners = {
            {{-halfWidth, -halfHeight, uv.U0, uv.V0},
             {halfWidth, -halfHeight, uv.U1, uv.V0},
             {halfWidth, halfHeight, uv.U1, uv.V1},
             {-halfWidth, halfHeight, uv.U0, uv.V1}}};
        constexpr int indices[] = {0, 1, 2, 2, 3, 0};
        for (int index : indices)
        {
            const auto& c = corners[index];
            Vertices.push_back({x + c[0] * cosine - c[1] * sine, y + c[0] * sine + c[1] * cosine,
                                c[2], c[3], color.R, color.G, color.B, color.A});
        }
        ++Stats.QuadCount;
    }
};

Renderer2D::Renderer2D() : m_Impl(std::make_unique<Impl>()) {}
Renderer2D::~Renderer2D()
{
    Shutdown();
}

bool Renderer2D::Initialize()
{
    auto& impl = *m_Impl;
    if (impl.Initialized)
        return true;
    impl.Error.clear();
    impl.Context = wglGetCurrentContext();
    if (!impl.Context)
    {
        impl.Fail("Renderer2D requires an active OpenGL context.");
        return false;
    }
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* device = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    int major = 0, minor = 0;
    if (version)
        sscanf_s(version, "%d.%d", &major, &minor);
    impl.Modern = major > 3 || (major == 3 && minor >= 3);
    if (impl.Modern && (!impl.GL.Load() || !impl.InitializeModern()))
    {
        if (impl.Error.empty())
            impl.Fail("The OpenGL 3.3 driver is missing required entry points.");
        Shutdown();
        return false;
    }
    impl.Backend =
        std::string(impl.Modern ? "OpenGL 3.3 batched" : "OpenGL compatibility batched") + " | " +
        (device ? device : "unknown renderer");
    if (!impl.Modern)
        VX_LOG_WARN("OpenGL 3.3 unavailable: using the documented OpenGL 1.1 compatibility batch "
                    "renderer.");
    impl.Vertices.reserve(MaxVertices);
    impl.Initialized = true;
    const std::uint8_t white[] = {255, 255, 255, 255};
    // GL 1.1 GL_CLAMP blends with the border around a one-texel texture;
    // repeating white keeps solid primitives uniformly opaque on that path.
    impl.White = CreateTexture(1, 1, white, true);
    if (!impl.White)
    {
        Shutdown();
        return false;
    }

    HDC dc = CreateCompatibleDC(nullptr);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = FontWidth;
    info.bmiHeader.biHeight = -FontHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap =
        dc ? CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0) : nullptr;
    HFONT font = CreateFontW(-FontPixels, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                             DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    if (!dc || !bitmap || !font)
    {
        if (font)
            DeleteObject(font);
        if (bitmap)
            DeleteObject(bitmap);
        if (dc)
            DeleteDC(dc);
        impl.Fail("Could not create the UI font atlas.");
        Shutdown();
        return false;
    }
    HGDIOBJ previousBitmap = SelectObject(dc, bitmap);
    HGDIOBJ previousFont = SelectObject(dc, font);
    std::memset(pixels, 0, FontWidth * FontHeight * 4);
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_LEFT | TA_TOP);
    for (int index = 0; index < 96; ++index)
    {
        const wchar_t character = static_cast<wchar_t>(index + 32);
        SIZE extent{};
        GetTextExtentPoint32W(dc, &character, 1, &extent);
        impl.GlyphAdvance[index] = static_cast<float>(extent.cx);
        TextOutW(dc, (index % 16) * FontCellWidth + FontMargin,
                 (index / 16) * FontCellHeight + FontMargin, &character, 1);
    }
    GdiFlush();
    const auto* bgra = static_cast<const std::uint8_t*>(pixels);
    std::vector<std::uint8_t> rgba(FontWidth * FontHeight * 4);
    for (std::size_t p = 0; p < rgba.size(); p += 4)
    {
        rgba[p] = rgba[p + 1] = rgba[p + 2] = 255;
        rgba[p + 3] = std::max({bgra[p], bgra[p + 1], bgra[p + 2]});
    }
    SelectObject(dc, previousFont);
    SelectObject(dc, previousBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
    impl.Font = CreateTexture(FontWidth, FontHeight, rgba.data());
    if (!impl.Font)
    {
        Shutdown();
        return false;
    }
    VX_LOG_INFO(impl.Backend);
    return true;
}

void Renderer2D::Shutdown()
{
    auto& impl = *m_Impl;
    impl.Vertices.clear();
    for (auto& weak : impl.Textures)
        if (auto texture = weak.lock())
            texture->Release();
    impl.Textures.clear();
    impl.Font.reset();
    impl.White.reset();
    if (impl.Context && wglGetCurrentContext() == impl.Context)
    {
        if (impl.VBO)
            impl.GL.DeleteBuffers(1, &impl.VBO);
        if (impl.VAO)
            impl.GL.DeleteVertexArrays(1, &impl.VAO);
        if (impl.Program)
            impl.GL.DeleteProgram(impl.Program);
    }
    impl.VBO = impl.VAO = impl.Program = impl.BatchTexture = 0;
    impl.Initialized = false;
    impl.Context = nullptr;
}

void Renderer2D::BeginFrame(int framebufferWidth, int framebufferHeight, float logicalWidth,
                            float logicalHeight, Color clear)
{
    auto& impl = *m_Impl;
    if (!impl.Initialized)
        return;
    impl.Vertices.clear();
    impl.Stats = {};
    impl.FramebufferWidth = std::max(framebufferWidth, 1);
    impl.FramebufferHeight = std::max(framebufferHeight, 1);
    impl.LogicalWidth = std::max(logicalWidth, 1.0f);
    impl.LogicalHeight = std::max(logicalHeight, 1.0f);
    impl.Camera = Camera2D{};
    impl.Camera.SetViewport(impl.LogicalWidth, impl.LogicalHeight);
    glViewport(0, 0, impl.FramebufferWidth, impl.FramebufferHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(clear.R, clear.G, clear.B, clear.A);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer2D::EndFrame()
{
    m_Impl->Flush();
}
void Renderer2D::SetCamera(const Camera2D& camera)
{
    m_Impl->Flush();
    m_Impl->Camera = camera;
}

void Renderer2D::SetClipRect(float x, float y, float width, float height)
{
    auto& impl = *m_Impl;
    if (!impl.Initialized)
        return;
    impl.Flush();
    glEnable(GL_SCISSOR_TEST);
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height) ||
        width <= 0 || height <= 0)
    {
        glScissor(0, 0, 0, 0);
        return;
    }
    const float scaleX = static_cast<float>(impl.FramebufferWidth) / impl.LogicalWidth;
    const float scaleY = static_cast<float>(impl.FramebufferHeight) / impl.LogicalHeight;
    const int left = static_cast<int>(std::floor(std::clamp(x, 0.0f, impl.LogicalWidth) * scaleX));
    const int right =
        static_cast<int>(std::ceil(std::clamp(x + width, 0.0f, impl.LogicalWidth) * scaleX));
    const int top = static_cast<int>(std::floor(std::clamp(y, 0.0f, impl.LogicalHeight) * scaleY));
    const int bottom =
        static_cast<int>(std::ceil(std::clamp(y + height, 0.0f, impl.LogicalHeight) * scaleY));
    glScissor(left, impl.FramebufferHeight - bottom, std::max(right - left, 0),
              std::max(bottom - top, 0));
}

void Renderer2D::ClearClipRect()
{
    if (!m_Impl->Initialized)
        return;
    m_Impl->Flush();
    glDisable(GL_SCISSOR_TEST);
}

void Renderer2D::DrawQuad(float x, float y, float width, float height, Color color,
                          const Texture2D* texture, float rotationRadians)
{
    if (!m_Impl->Initialized)
        return;
    const auto handle = texture && texture->GetNativeHandle() ? texture->GetNativeHandle()
                                                              : m_Impl->White->GetNativeHandle();
    m_Impl->Quad(x, y, width, height, color, handle, {}, rotationRadians);
}

void Renderer2D::DrawSprite(float x, float y, float width, float height, const Texture2D& texture,
                            UVRect uv, Color tint, float rotationRadians)
{
    if (!texture.GetNativeHandle())
        return;
    m_Impl->Quad(x, y, width, height, tint, texture.GetNativeHandle(), uv, rotationRadians);
}

void Renderer2D::DrawLine(float x1, float y1, float x2, float y2, Color color, float thickness)
{
    const float dx = x2 - x1, dy = y2 - y1;
    DrawQuad((x1 + x2) * 0.5f, (y1 + y2) * 0.5f, std::sqrt(dx * dx + dy * dy), thickness, color,
             nullptr, std::atan2(dy, dx));
}

void Renderer2D::DrawRect(float x, float y, float width, float height, Color color, float thickness)
{
    const float left = x - width * 0.5f, right = x + width * 0.5f;
    const float top = y - height * 0.5f, bottom = y + height * 0.5f;
    DrawLine(left, top, right, top, color, thickness);
    DrawLine(right, top, right, bottom, color, thickness);
    DrawLine(right, bottom, left, bottom, color, thickness);
    DrawLine(left, bottom, left, top, color, thickness);
}

void Renderer2D::DrawText(std::string_view text, float x, float y, float pixelSize, Color color)
{
    auto& impl = *m_Impl;
    if (!impl.Initialized || !impl.Font || pixelSize <= 0)
        return;
    const float originX = x, scale = pixelSize / FontPixels;
    for (unsigned char character : text)
    {
        if (character == '\n')
        {
            x = originX;
            y += pixelSize * 1.35f;
            continue;
        }
        if (character == '\r')
            continue;
        if (character == '\t')
        {
            x += impl.GlyphAdvance[0] * scale * 4;
            continue;
        }
        if (character < 32 || character > 126)
            character = '?';
        const int index = character - 32;
        if (character != ' ')
        {
            const float u = static_cast<float>((index % 16) * FontCellWidth) / FontWidth;
            const float v = static_cast<float>((index / 16) * FontCellHeight) / FontHeight;
            const UVRect uv{u, v, u + static_cast<float>(FontCellWidth) / FontWidth,
                            v + static_cast<float>(FontCellHeight) / FontHeight};
            impl.Quad(x + (FontCellWidth * 0.5f - FontMargin) * scale,
                      y + (FontCellHeight * 0.5f - FontMargin) * scale, FontCellWidth * scale,
                      FontCellHeight * scale, color, impl.Font->GetNativeHandle(), uv, 0);
        }
        x += impl.GlyphAdvance[index] * scale;
    }
}

float Renderer2D::MeasureText(std::string_view text, float pixelSize) const
{
    float width = 0, longest = 0;
    for (unsigned char character : text)
    {
        if (character == '\n')
        {
            longest = std::max(longest, width);
            width = 0;
            continue;
        }
        if (character == '\r')
            continue;
        if (character == '\t')
        {
            width += m_Impl->GlyphAdvance[0] * 4;
            continue;
        }
        if (character < 32 || character > 126)
            character = '?';
        width += m_Impl->GlyphAdvance[character - 32];
    }
    return std::max(longest, width) * std::max(pixelSize, 0.0f) / FontPixels;
}

std::shared_ptr<Texture2D> Renderer2D::CreateTexture(int width, int height,
                                                     const std::uint8_t* rgba, bool repeat,
                                                     bool linear)
{
    auto& impl = *m_Impl;
    if (!impl.Initialized || !rgba || width <= 0 || height <= 0)
        return {};
    impl.Flush();
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    if (maxSize <= 0)
    {
        impl.Fail("OpenGL reported an invalid maximum texture size.");
        return {};
    }
    const int uploadWidth = std::min(impl.Modern ? width : PowerOfTwo(width), maxSize);
    const int uploadHeight = std::min(impl.Modern ? height : PowerOfTwo(height), maxSize);
    std::vector<std::uint8_t> resized;
    if (uploadWidth != width || uploadHeight != height)
    {
        resized.resize(static_cast<std::size_t>(uploadWidth) * uploadHeight * 4);
        for (int y = 0; y < uploadHeight; ++y)
            for (int x = 0; x < uploadWidth; ++x)
            {
                const std::size_t source =
                    (static_cast<std::size_t>(y) * height / uploadHeight * width +
                     static_cast<std::size_t>(x) * width / uploadWidth) *
                    4;
                const std::size_t target = (static_cast<std::size_t>(y) * uploadWidth + x) * 4;
                std::memcpy(resized.data() + target, rgba + source, 4);
            }
        rgba = resized.data();
    }
    GLuint handle = 0;
    glGenTextures(1, &handle);
    if (impl.Modern)
        impl.GL.ActiveTexture(Texture0);
    glBindTexture(GL_TEXTURE_2D, handle);
    const GLint wrap = repeat ? GL_REPEAT : (impl.Modern ? ClampToEdge : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, uploadWidth, uploadHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 rgba);
    if (glGetError() != GL_NO_ERROR)
    {
        glDeleteTextures(1, &handle);
        impl.Fail("OpenGL texture upload failed.");
        return {};
    }
    auto result = std::shared_ptr<Texture2D>(new Texture2D(handle, width, height, impl.Context));
    impl.Textures.emplace_back(result);
    return result;
}

std::shared_ptr<Texture2D> Renderer2D::LoadTexture(const std::filesystem::path& path, bool repeat,
                                                   bool linear)
{
    using Microsoft::WRL::ComPtr;
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(apartment) && apartment != RPC_E_CHANGED_MODE)
    {
        m_Impl->Fail("Could not initialize Windows image decoding.");
        return {};
    }
    std::vector<std::uint8_t> pixels;
    UINT width = 0, height = 0;
    const HRESULT result = [&]() -> HRESULT
    {
        ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(factory.GetAddressOf()));
        if (FAILED(hr))
            return hr;
        ComPtr<IWICBitmapDecoder> decoder;
        hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                WICDecodeMetadataCacheOnLoad,
                                                decoder.GetAddressOf());
        if (FAILED(hr))
            return hr;
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, frame.GetAddressOf());
        if (FAILED(hr))
            return hr;
        hr = frame->GetSize(&width, &height);
        if (FAILED(hr))
            return hr;
        if (!width || !height || width > 16384 || height > 16384)
            return E_INVALIDARG;
        ComPtr<IWICFormatConverter> converter;
        hr = factory->CreateFormatConverter(converter.GetAddressOf());
        if (FAILED(hr))
            return hr;
        hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                   WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
            return hr;
        pixels.resize(static_cast<std::size_t>(width) * height * 4);
        return converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()),
                                     pixels.data());
    }();
    if (SUCCEEDED(apartment))
        CoUninitialize();
    if (FAILED(result))
    {
        m_Impl->Fail("Could not load texture: " + path.string() + " (Windows image decoder error " +
                     std::to_string(static_cast<unsigned long>(result)) + ")");
        return {};
    }
    return CreateTexture(static_cast<int>(width), static_cast<int>(height), pixels.data(), repeat,
                         linear);
}

bool Renderer2D::CapturePPM(const std::filesystem::path& path)
{
    auto& impl = *m_Impl;
    if (!impl.Initialized || impl.FramebufferWidth <= 0 || impl.FramebufferHeight <= 0)
        return false;
    impl.Flush();
    glFinish();
    const std::size_t stride = static_cast<std::size_t>(impl.FramebufferWidth) * 3;
    std::vector<std::uint8_t> pixels(stride * impl.FramebufferHeight);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, impl.FramebufferWidth, impl.FramebufferHeight, GL_RGB, GL_UNSIGNED_BYTE,
                 pixels.data());
    if (glGetError() != GL_NO_ERROR)
    {
        impl.Fail("OpenGL screenshot readback failed.");
        return false;
    }
    std::ofstream output(path, std::ios::binary);
    if (!output)
    {
        impl.Fail("Could not open screenshot output: " + path.string());
        return false;
    }
    output << "P6\n" << impl.FramebufferWidth << ' ' << impl.FramebufferHeight << "\n255\n";
    for (int y = impl.FramebufferHeight - 1; y >= 0; --y)
        output.write(
            reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y) * stride),
            static_cast<std::streamsize>(stride));
    return output.good();
}

const Renderer2DStats& Renderer2D::GetStats() const
{
    return m_Impl->Stats;
}
const std::string& Renderer2D::GetBackendName() const
{
    return m_Impl->Backend;
}
const std::string& Renderer2D::GetLastError() const
{
    return m_Impl->Error;
}
} // namespace vx
