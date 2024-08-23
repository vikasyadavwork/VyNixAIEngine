#pragma once

// ------------------------------------------------------------
// Compiler
// ------------------------------------------------------------

#if defined(_MSC_VER)
#define VX_COMPILER_MSVC
#elif defined(__clang__)
#define VX_COMPILER_CLANG
#elif defined(__GNUC__)
#define VX_COMPILER_GCC
#else
#error Unsupported compiler
#endif

// ------------------------------------------------------------
// Build Configuration
// ------------------------------------------------------------

#ifdef NDEBUG
#define VX_RELEASE
#else
#define VX_DEBUG
#endif

// ------------------------------------------------------------
// Utility
// ------------------------------------------------------------

#define VX_BIT(x) (1 << (x))
#define VX_UNUSED(x) (void)(x)

#if defined(_MSC_VER)
#define VX_FORCE_INLINE __forceinline
#else
#define VX_FORCE_INLINE inline __attribute__((always_inline))
#endif

//Logging
#define VX_LOG_TRACE(msg) \
    ::vx::Logger::Get().Log(::vx::LogLevel::Trace, msg)

#define VX_LOG_DEBUG(msg) \
    ::vx::Logger::Get().Log(::vx::LogLevel::Debug, msg)

#define VX_LOG_INFO(msg) \
    ::vx::Logger::Get().Log(::vx::LogLevel::Info, msg)

#define VX_LOG_WARN(msg) \
    ::vx::Logger::Get().Log(::vx::LogLevel::Warn, msg)

#define VX_LOG_ERROR(msg) \
    ::vx::Logger::Get().Log(::vx::LogLevel::Error, msg)

#define VX_LOG_FATAL(msg) \
    ::vx::Logger::Get().Log(::vx::LogLevel::Fatal, msg)