#pragma once
#include <vector>
#include <vx/Scene/Scene.h>

namespace skybound
{
enum class State
{
    Ready,
    Playing,
    Paused,
    Won,
    Lost
};
enum class Difficulty
{
    Cadet,
    Pilot,
    Ace
};
struct Plane
{
    vx::Vec2 position;
    int health = 100;
    float flash = 0;
};
struct Bullet
{
    vx::Vec2 position, velocity;
    bool player = true;
};
struct GameEvent
{
    enum class Kind
    {
        Shot,
        Hit,
        Won,
        Lost
    };
    Kind kind;
    vx::Vec2 position;
    bool player;
};
struct Controls
{
    vx::Vec2 movement{};
    bool fire = false;
};

// Pure simulation: no window, renderer or sound dependency. Fixed-step friendly.
class Game
{
  public:
    Plane player{{210, 337}}, enemy{{990, 337}};
    std::vector<Bullet> bullets;
    std::vector<GameEvent> events;
    State state = State::Ready;
    Difficulty difficulty = Difficulty::Pilot;
    float elapsed = 0;
    int shots = 0, hits = 0;
    void Start(Difficulty value = Difficulty::Pilot);
    void Pause();
    void Update(float dt, Controls controls);
    int Score() const;
    int Accuracy() const;

  private:
    float m_PlayerCooldown = 0, m_EnemyCooldown = 1.4f;
};
} // namespace skybound
