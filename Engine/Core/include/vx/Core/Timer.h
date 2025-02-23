#pragma once

namespace vx
{

class Timer
{
  public:
    Timer();

    void Reset();
    void Update();

    float GetDeltaTime() const noexcept;
    float GetElapsedTime() const noexcept;

  private:
    float m_DeltaTime;
    float m_ElapsedTime;
    double m_LastTime;
};

} // namespace vx
