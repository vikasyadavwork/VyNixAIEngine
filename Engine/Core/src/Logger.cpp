#include <vx/Core/ConsoleSink.h>
#include <vx/Core/Logger.h>

#include <utility>

namespace vx
{

Logger& Logger::Get()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
{
    AddSink(std::make_unique<ConsoleSink>());
}

void Logger::AddSink(std::unique_ptr<ILogSink> sink)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_Sinks.emplace_back(std::move(sink));
}

void Logger::Log(LogLevel level, std::string_view message)
{
    LogMessage logMessage(level, message);

    std::lock_guard<std::mutex> lock(m_Mutex);

    for (auto& sink : m_Sinks)
    {
        sink->Write(logMessage);
    }
}

} // namespace vx