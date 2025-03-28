#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

// Windows defines DrawText as a macro in windows.h. Keep the renderer API stable
// when this header is included after the platform headers.
#ifdef DrawText
#undef DrawText
#endif

namespace vx
{
struct Color
{
    float R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f;
};

// Normalized texture coordinates; V=0 is the top of an image loaded by this renderer.
struct UVRect
{
    float U0 = 0.0f, V0 = 0.0f, U1 = 1.0f, V1 = 1.0f;
};

class Camera2D
{
  public:
    void SetViewport(float width, float height);
    void SetPosition(float x, float y)
    {
        m_X = x;
        m_Y = y;
    }
    void SetZoom(float zoom);
    [[nodiscard]] std::array<float, 16> GetProjection() const;

  private:
    float m_Width = 1280.0f, m_Height = 720.0f;
    float m_X = 0.0f, m_Y = 0.0f, m_Zoom = 1.0f;
};

class Texture2D
{
  public:
    ~Texture2D();
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    [[nodiscard]] int GetWidth() const
    {
        return m_Width;
    }
    [[nodiscard]] int GetHeight() const
    {
        return m_Height;
    }
    [[nodiscard]] std::uint32_t GetNativeHandle() const
    {
        return m_Handle;
    }

  private:
    friend class Renderer2D;
    Texture2D(std::uint32_t handle, int width, int height, void* context);
    void Release();
    std::uint32_t m_Handle = 0;
    int m_Width = 0, m_Height = 0;
    void* m_Context = nullptr;
};

struct Renderer2DStats
{
    std::uint32_t DrawCalls = 0;
    std::uint32_t QuadCount = 0;
    std::uint32_t VertexCount = 0;
    std::uint32_t TextureBinds = 0;
};

// A current WGL context is required for initialization, drawing and texture loading.
// All positions are logical pixels, Y increases downward, and quads rotate about their center.
class Renderer2D
{
  public:
    Renderer2D();
    ~Renderer2D();
    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;

    bool Initialize();
    void Shutdown();
    void BeginFrame(int framebufferWidth, int framebufferHeight, float logicalWidth = 1280.0f,
                    float logicalHeight = 720.0f, Color clear = {0.025f, 0.045f, 0.08f, 1.0f});
    void EndFrame();
    void SetCamera(const Camera2D& camera);
    // Clip in logical screen coordinates, independent of the camera. Position is top-left.
    // Each call flushes pending drawing. BeginFrame restores the unclipped viewport.
    void SetClipRect(float x, float y, float width, float height);
    void ClearClipRect();
    void DrawQuad(float x, float y, float width, float height, Color color,
                  const Texture2D* texture = nullptr, float rotationRadians = 0.0f);
    void DrawSprite(float x, float y, float width, float height, const Texture2D& texture,
                    UVRect uv = {}, Color tint = {}, float rotationRadians = 0.0f);
    void DrawLine(float x1, float y1, float x2, float y2, Color color, float thickness = 1.0f);
    void DrawRect(float x, float y, float width, float height, Color color, float thickness = 1.0f);
    // Text uses a top-left position, supports ASCII and newline, and scales from a generated font
    // atlas.
    void DrawText(std::string_view text, float x, float y, float pixelSize, Color color = {});
    [[nodiscard]] float MeasureText(std::string_view text, float pixelSize) const;
    [[nodiscard]] std::shared_ptr<Texture2D> LoadTexture(const std::filesystem::path& path,
                                                         bool repeat = false, bool linear = true);
    [[nodiscard]] std::shared_ptr<Texture2D> CreateTexture(int width, int height,
                                                           const std::uint8_t* rgba,
                                                           bool repeat = false, bool linear = true);
    // Call after EndFrame and before the window swaps buffers.
    bool CapturePPM(const std::filesystem::path& path);
    [[nodiscard]] const Renderer2DStats& GetStats() const;
    [[nodiscard]] const std::string& GetBackendName() const;
    [[nodiscard]] const std::string& GetLastError() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};
} // namespace vx
