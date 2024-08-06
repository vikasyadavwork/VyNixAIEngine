#pragma once

#include <vx/Core/LogMessage.h>

namespace vx
{

class ILogSink
{
  public:
    virtual ~ILogSink() = default;

    virtual void Write(const LogMessage& message) = 0;
};

} // namespace vx