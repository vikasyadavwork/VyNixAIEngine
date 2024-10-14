#pragma once

#include <vx/Core/Logger.h>

// Logging
#define VX_LOG_TRACE(msg) ::vx::Logger::Get().Log(::vx::LogLevel::Trace, msg)

#define VX_LOG_DEBUG(msg) ::vx::Logger::Get().Log(::vx::LogLevel::Debug, msg)

#define VX_LOG_INFO(msg) ::vx::Logger::Get().Log(::vx::LogLevel::Info, msg)

#define VX_LOG_WARN(msg) ::vx::Logger::Get().Log(::vx::LogLevel::Warn, msg)

#define VX_LOG_ERROR(msg) ::vx::Logger::Get().Log(::vx::LogLevel::Error, msg)

#define VX_LOG_FATAL(msg) ::vx::Logger::Get().Log(::vx::LogLevel::Fatal, msg)