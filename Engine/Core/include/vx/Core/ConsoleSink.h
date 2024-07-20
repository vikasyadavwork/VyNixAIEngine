#pragma once

#include <vx/Core/ILogSink.h>

namespace vx
{

class ConsoleSink : public ILogSink
{
  public:
    void Write(const LogMessage& message) override;
};

} // namespace vx