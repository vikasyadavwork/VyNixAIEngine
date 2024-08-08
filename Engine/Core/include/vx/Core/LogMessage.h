#pragma once

#include <string>
#include <string_view>

#include <vx/Core/LogLevel.h>

namespace vx
{

struct LogMessage
{
    LogMessage(LogLevel level, std::string_view message) : Level(level), Message(message) {}
    LogLevel Level;

    std::string Message;
};

} // namespace vx