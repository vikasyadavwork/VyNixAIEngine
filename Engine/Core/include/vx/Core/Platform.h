#pragma once

//============================================================
// Platform Detection
//============================================================

#if defined(_WIN32) || defined(_WIN64)

#define VX_PLATFORM_WINDOWS

#elif defined(__linux__)

#define VX_PLATFORM_LINUX

#elif defined(__APPLE__)

#define VX_PLATFORM_MACOS

#else

#error Unsupported platform

#endif

namespace vx
{

class Platform
{
};

} // namespace vx