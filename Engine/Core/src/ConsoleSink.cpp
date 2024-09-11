#include <iostream>

#include <vx/Core/ConsoleSink.h>

namespace vx
{

void ConsoleSink::Write(const LogMessage& message)
{
    switch (message.Level)
    {
    case LogLevel::Trace:
        std::cout << "[TRACE] ";
        break;

    case LogLevel::Debug:
        std::cout << "[DEBUG] ";
        break;

    case LogLevel::Info:
        std::cout << "[INFO ] ";
        break;

    case LogLevel::Warn:
        std::cout << "[WARN ] ";
        break;

    case LogLevel::Error:
        std::cout << "[ERROR] ";
        break;

    case LogLevel::Fatal:
        std::cout << "[FATAL] ";
        break;
    }

    std::cout << message.Message << '\n';
}

} // namespace vx