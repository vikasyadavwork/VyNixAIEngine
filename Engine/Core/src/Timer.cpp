#include "vx/Core/Timer.h"
#include "vx/Core/Log.h"

#include <chrono>

namespace
{
double GetCurrentTime()
{
    using Clock = std::chrono::steady_clock;

    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}
} // namespace

namespace vx
{

Timer::Timer()
{
    Reset();
}

void Timer::Reset()
{
    m_LastTime = GetCurrentTime();
    m_DeltaTime = 0.0f;
    m_ElapsedTime = 0.0f;
}

void Timer::Update()
{
    double current = GetCurrentTime();

    m_DeltaTime = static_cast<float>(current - m_LastTime);
    m_ElapsedTime += m_DeltaTime;

    m_LastTime = current;
}

float Timer::GetDeltaTime() const noexcept
{
    return m_DeltaTime;
}

float Timer::GetElapsedTime() const noexcept
{
    return m_ElapsedTime;
}

} // namespace vx