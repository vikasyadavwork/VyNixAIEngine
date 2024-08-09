#pragma once

#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

#include <vx/Core/ILogSink.h>
#include <vx/Core/LogLevel.h>

namespace vx
{

class Logger
{
  public:
    static Logger& Get();
    void Log(LogLevel level, std::string_view messsage);
    void AddSink(std::unique_ptr<ILogSink> sink);

  private:
    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

  private:
    std::vector<std::unique_ptr<ILogSink>> m_Sinks;
    std::mutex m_Mutex;
};

} // namespace vx