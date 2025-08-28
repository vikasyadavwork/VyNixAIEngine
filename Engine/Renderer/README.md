# Renderer2D

`vx-renderer` is the reusable C++20 renderer. Include `<vx/Renderer/Renderer2D.h>`.
It requires a current Windows OpenGL context. `WGLContext` requests an OpenGL 3.3
core profile; `Renderer2D` loads the required entry points and uses shader programs,
a vertex array, and a streaming vertex buffer. Consecutive quads using the same
texture are submitted in one batch; a texture or camera change flushes that batch.
The per-frame statistics report the actual batch draw calls and submitted quads.

If a Windows software or Remote Desktop driver cannot create an OpenGL 3.3
context, the existing compatibility context remains active. The renderer then
uses the OpenGL 1.1 client vertex-array path with the same public drawing API.
This fallback logs a warning and identifies itself in `GetBackendName()`.
It resizes textures to supported power-of-two dimensions and does not use shaders.
OpenGL 3.3 shader or entry-point failures are reported as initialization errors;
they do not silently switch a core context to unsupported legacy calls.

## Coordinates and drawing

`BeginFrame(framebufferWidth, framebufferHeight, logicalWidth, logicalHeight)`
sets the viewport to the physical client size and maps the logical drawing area
to it. The default logical area is 1280 by 720. Positive Y points down, and quad
positions are their centers. Positive rotation is clockwise in radians. Text
positions refer to the top-left of their font layout. `DrawSprite` accepts a
normalized `UVRect`, with V=0 at the top of an image, for sprites within an atlas.
`Camera2D` provides an orthographic viewport, origin, and zoom; changing it flushes
pending drawing. `BeginFrame` restores the default camera for the new frame.

`SetClipRect(x, y, width, height)` limits subsequent drawing to a rectangle whose
position is its top-left in logical screen coordinates, independent of the camera.
It scales the rectangle to the physical framebuffer and converts the Y origin for
OpenGL scissoring. `ClearClipRect()` restores drawing across the viewport. Both
operations flush pending batches, and `BeginFrame` clears the previous frame's clip.

`DrawText` uses an antialiased Segoe UI bitmap atlas generated with Windows GDI.
It supports printable ASCII, tabs, and newlines, and scales to logical pixel sizes.
`MeasureText` returns the width of the longest line at the requested size.

## Textures and lifetime

`LoadTexture` decodes PNG and other Windows Imaging Component formats into RGBA.
No image decoder DLL or font file is shipped with the application. Linear
filtering is the default; repeating UVs can be enabled when loading or creating
a texture. `CreateTexture` accepts tightly packed, top-to-bottom RGBA8 pixels.
Images above the driver's maximum texture size are resized during upload.

Textures have move-only ownership and are returned through `shared_ptr` for scene
sharing. Perform drawing and resource operations on the context's owning thread.
Call `Shutdown` before destroying the window/context; this also releases GPU
handles of any externally retained textures. A texture retained across shutdown
must be loaded again after renderer reinitialization.

Call `EndFrame` to submit pending geometry, then swap the window buffers.
`CapturePPM(path)` reads the current back buffer, so call it after `EndFrame` and
before swapping. It writes a top-to-bottom binary PPM image for reproducible
smoke-test inspection. Initialization, texture, and capture failures are exposed
through return values and `GetLastError()`.
