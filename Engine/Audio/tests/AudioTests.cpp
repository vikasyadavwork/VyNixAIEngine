#include <chrono>
#include <iostream>
#include <thread>
#include <vx/Audio/Audio.h>

int main()
{
    vx::Audio::SetMuted(true);
    const bool available = vx::Audio::Initialize();
    if (!vx::Audio::IsMuted())
        return 1;
    vx::Audio::SetEngineRunning(true);
    vx::Audio::Play(vx::SoundEffect::MachineGun);
    std::this_thread::sleep_for(std::chrono::milliseconds(65));
    if (available != vx::Audio::IsAvailable())
        return 2;
    vx::Audio::SetEngineRunning(false);
    vx::Audio::Shutdown();
    vx::Audio::Shutdown();
    if (vx::Audio::IsAvailable())
        return 3;
    vx::Audio::Play(vx::SoundEffect::Hit);
    const bool reopened = vx::Audio::Initialize();
    if (available && !reopened)
        return 4;
    vx::Audio::Shutdown();
    vx::Audio::SetMuted(false);
    std::cout << "Audio buffer lifecycle and silent fallback checks passed. Device: "
              << (available ? "available" : "unavailable (silent mode)") << '\n';
    return 0;
}
