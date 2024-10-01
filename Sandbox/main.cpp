#include <vx/Core/Core.h>

int main()
{
    VX_LOG_TRACE("Starting Engine");

    VX_LOG_DEBUG("Loading Renderer");

    VX_LOG_INFO("Engine Initialized");

    VX_LOG_WARN("Texture Missing");

    VX_LOG_ERROR("Failed to Load Shader");

    VX_LOG_FATAL("GPU Device Lost");
}