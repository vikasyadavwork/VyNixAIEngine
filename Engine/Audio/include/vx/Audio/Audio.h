#pragma once

namespace vx
{
enum class SoundEffect
{
    MachineGun,
    EnemyGun,
    Hit,
    Explosion,
    Victory
};

// Native, asynchronous WinMM synthesizer. No sound assets or blocking tones.
// Call Initialize/Shutdown on the application thread. Unavailable audio is a
// supported silent mode; Play and control calls remain safe in that state.
class Audio
{
  public:
    static bool Initialize();
    static void Shutdown();
    static bool IsAvailable() noexcept;
    static void SetMuted(bool muted) noexcept;
    static bool IsMuted() noexcept;
    static void SetEngineRunning(bool running) noexcept;
    static void Play(SoundEffect effect);
};
} // namespace vx
