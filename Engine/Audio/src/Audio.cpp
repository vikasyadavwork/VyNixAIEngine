#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mmsystem.h>
#include <mutex>
#include <thread>
#include <vector>
#include <vx/Audio/Audio.h>
#include <vx/Core/Log.h>

namespace
{
constexpr unsigned SampleRate = 22050;
constexpr unsigned BufferSamples = 256;
constexpr double Tau = 6.283185307179586;

struct Voice
{
    vx::SoundEffect effect;
    double age = 0, phase = 0;
    double duration = .08, startFrequency = 900, endFrequency = 100;
    double volume = .25, noise = .2;
};

struct Buffer
{
    WAVEHDR header{};
    std::array<int16_t, BufferSamples> samples{};
    bool prepared = false;
};

class Mixer
{
  public:
    ~Mixer()
    {
        Stop();
    }

    bool Start()
    {
        event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event)
            return false;
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = 1;
        format.nSamplesPerSec = SampleRate;
        format.wBitsPerSample = 16;
        format.nBlockAlign = 2;
        format.nAvgBytesPerSec = SampleRate * 2;
        if (waveOutOpen(&device, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(event), 0,
                        CALLBACK_EVENT) != MMSYSERR_NOERROR)
            return false;
        for (auto& buffer : buffers)
        {
            buffer.header.lpData = reinterpret_cast<LPSTR>(buffer.samples.data());
            buffer.header.dwBufferLength = sizeof(buffer.samples);
            if (waveOutPrepareHeader(device, &buffer.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
                return false;
            buffer.prepared = true;
            if (waveOutWrite(device, &buffer.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
                return false;
        }
        running = true;
        worker = std::thread([this] { Run(); });
        return true;
    }

    void Stop()
    {
        running = false;
        if (event)
            SetEvent(event);
        if (worker.joinable())
            worker.join();
        if (device)
        {
            waveOutReset(device);
            for (auto& buffer : buffers)
                if (buffer.prepared)
                    waveOutUnprepareHeader(device, &buffer.header, sizeof(WAVEHDR));
            waveOutClose(device);
            device = nullptr;
        }
        if (event)
        {
            CloseHandle(event);
            event = nullptr;
        }
    }

    void Add(vx::SoundEffect effect)
    {
        if (muted)
            return;
        Voice voice{effect};
        switch (effect)
        {
        case vx::SoundEffect::MachineGun:
            break;
        case vx::SoundEffect::EnemyGun:
            voice.startFrequency = 530;
            voice.endFrequency = 80;
            voice.volume = .19;
            break;
        case vx::SoundEffect::Hit:
            voice.duration = .17;
            voice.startFrequency = 190;
            voice.endFrequency = 40;
            voice.volume = .35;
            voice.noise = .7;
            break;
        case vx::SoundEffect::Explosion:
            voice.duration = .8;
            voice.startFrequency = 90;
            voice.endFrequency = 20;
            voice.volume = .55;
            voice.noise = .85;
            break;
        case vx::SoundEffect::Victory:
            voice.duration = .75;
            voice.startFrequency = 440;
            voice.endFrequency = 880;
            voice.volume = .26;
            voice.noise = 0;
            break;
        }
        std::scoped_lock lock(voiceMutex);
        if (voices.size() >= 32)
            voices.erase(voices.begin());
        voices.push_back(voice);
    }

    std::atomic<bool> muted{false};
    std::atomic<bool> engine{false};

  private:
    double Noise()
    {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return (randomState / static_cast<double>(UINT32_MAX)) * 2 - 1;
    }

    void Fill(Buffer& buffer)
    {
        std::scoped_lock lock(voiceMutex);
        const bool silent = muted.load();
        const double targetEngine = engine.load() && !silent ? 1.0 : 0.0;
        for (auto& sample : buffer.samples)
        {
            engineGain += std::clamp(targetEngine - engineGain, -.002, .002);
            const double t = static_cast<double>(sampleCounter++) / SampleRate;
            double value = engineGain * .055 *
                           (std::sin(Tau * 65 * t) + .22 * std::sin(Tau * 130 * t)) *
                           (.75 + .25 * std::sin(Tau * 13 * t));
            for (auto& voice : voices)
            {
                if (voice.age >= voice.duration)
                    continue;
                const double progress = voice.age / voice.duration;
                double frequency =
                    voice.startFrequency + (voice.endFrequency - voice.startFrequency) * progress;
                if (voice.effect == vx::SoundEffect::Victory)
                {
                    constexpr std::array<int, 4> notes{0, 4, 7, 12};
                    const auto note = std::min(3, static_cast<int>(voice.age / .15));
                    frequency = 440 * std::pow(2.0, notes[note] / 12.0);
                }
                voice.phase += Tau * frequency / SampleRate;
                const double tonal =
                    .7 * std::sin(voice.phase) + .3 * (std::sin(voice.phase * .5) >= 0 ? 1 : -1);
                const double wave = tonal * (1 - voice.noise) + Noise() * voice.noise;
                const double envelope =
                    std::min(1.0, voice.age * 800) * std::pow(1 - progress, 1.7);
                value += wave * envelope * voice.volume;
                voice.age += 1.0 / SampleRate;
            }
            sample = silent ? 0 : static_cast<int16_t>(std::clamp(value, -.95, .95) * 32767);
        }
        std::erase_if(voices, [](const Voice& voice) { return voice.age >= voice.duration; });
    }

    void Run()
    {
        while (running)
        {
            WaitForSingleObject(event, 20);
            if (!running)
                break;
            for (auto& buffer : buffers)
            {
                if (buffer.header.dwFlags & WHDR_DONE)
                {
                    Fill(buffer);
                    if (waveOutWrite(device, &buffer.header, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
                    {
                        running = false;
                        break;
                    }
                }
            }
        }
    }

    HWAVEOUT device = nullptr;
    HANDLE event = nullptr;
    std::array<Buffer, 4> buffers{};
    std::atomic<bool> running{false};
    std::thread worker;
    std::mutex voiceMutex;
    std::vector<Voice> voices;
    uint32_t randomState = 0xAF136A21;
    uint64_t sampleCounter = 0;
    double engineGain = 0;
};

std::unique_ptr<Mixer> mixer;
bool requestedMuted = false;
} // namespace

namespace vx
{
bool Audio::Initialize()
{
    if (mixer)
        return true;
    auto candidate = std::make_unique<Mixer>();
    candidate->muted = requestedMuted;
    if (!candidate->Start())
    {
        VX_LOG_WARN("Audio device unavailable; continuing without sound.");
        return false;
    }
    mixer = std::move(candidate);
    VX_LOG_INFO("Native WinMM audio synthesizer ready.");
    return true;
}
void Audio::Shutdown()
{
    mixer.reset();
}
bool Audio::IsAvailable() noexcept
{
    return static_cast<bool>(mixer);
}
void Audio::SetMuted(bool muted) noexcept
{
    requestedMuted = muted;
    if (mixer)
        mixer->muted = muted;
}
bool Audio::IsMuted() noexcept
{
    return requestedMuted;
}
void Audio::SetEngineRunning(bool running) noexcept
{
    if (mixer)
        mixer->engine = running;
}
void Audio::Play(SoundEffect effect)
{
    if (mixer)
        mixer->Add(effect);
}
} // namespace vx
