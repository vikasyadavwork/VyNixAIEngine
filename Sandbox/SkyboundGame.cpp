#include "SkyboundGame.h"
#include <algorithm>
#include <cmath>

namespace skybound
{
void Game::Start(Difficulty value)
{
    difficulty = value;
    state = State::Playing;
    player = {{210, 337}, 100, 0};
    enemy = {{990, 337}, 100, 0};
    bullets.clear();
    events.clear();
    elapsed = 0;
    shots = hits = 0;
    m_PlayerCooldown = 0;
    m_EnemyCooldown = 1.4f;
}
void Game::Pause()
{
    if (state == State::Playing)
        state = State::Paused;
    else if (state == State::Paused)
        state = State::Playing;
}
void Game::Update(float dt, Controls controls)
{
    events.clear();
    if (state != State::Playing || !std::isfinite(dt) || dt <= 0 || dt > 0.1f)
        return;
    elapsed += dt;
    const int mode = static_cast<int>(difficulty);
    const float speed[3] = {90, 140, 185}, rate[3] = {1.1f, .7f, .48f},
                bulletSpeed[3] = {370, 450, 550};
    const int damage[3] = {6, 8, 10};
    const float length = std::hypot(controls.movement.x, controls.movement.y);
    if (length > 1)
        controls.movement = controls.movement / length;
    player.position.x = std::clamp(player.position.x + controls.movement.x * 310 * dt, 65.f, 555.f);
    player.position.y =
        std::clamp(player.position.y + controls.movement.y * 310 * dt, 100.f, 585.f);
    const float target =
        std::clamp(player.position.y + std::sin(elapsed * 1.7f) * 85, 100.f, 585.f);
    enemy.position.y += std::clamp(target - enemy.position.y, -speed[mode] * dt, speed[mode] * dt);
    enemy.position.x = 960 + std::sin(elapsed * .8f) * 70;
    player.flash = std::max(0.f, player.flash - dt);
    enemy.flash = std::max(0.f, enemy.flash - dt);
    m_PlayerCooldown -= dt;
    m_EnemyCooldown -= dt;
    if (controls.fire && m_PlayerCooldown <= 0)
    {
        m_PlayerCooldown = .16f;
        shots++;
        vx::Vec2 p = player.position + vx::Vec2{55, 0};
        bullets.push_back({p, {720, 0}, true});
        events.push_back({GameEvent::Kind::Shot, p, true});
    }
    if (m_EnemyCooldown <= 0)
    {
        m_EnemyCooldown = rate[mode];
        vx::Vec2 d = player.position - enemy.position;
        const float n = std::max(1.f, std::hypot(d.x, d.y));
        vx::Vec2 p = enemy.position + vx::Vec2{-55, 0};
        bullets.push_back({p, d * (bulletSpeed[mode] / n), false});
        events.push_back({GameEvent::Kind::Shot, p, false});
    }
    std::vector<Bullet> remaining;
    remaining.reserve(bullets.size());
    for (auto b : bullets)
    {
        const auto previous = b.position;
        b.position += b.velocity * dt;
        auto& plane = b.player ? enemy : player;
        if (vx::sweepSegment(previous, b.position, {plane.position.x, plane.position.y, 82, 68}))
        {
            plane.health = std::max(0, plane.health - (b.player ? 10 : damage[mode]));
            plane.flash = .13f;
            if (b.player)
                hits++;
            events.push_back({GameEvent::Kind::Hit, b.position, b.player});
        }
        else if (b.position.x > -30 && b.position.x < 1230 && b.position.y > -30 &&
                 b.position.y < 705)
            remaining.push_back(b);
    }
    bullets.swap(remaining);
    // If both planes are hit fatally in one simulation step, the player's mission fails.
    if (player.health <= 0)
    {
        state = State::Lost;
        events.push_back({GameEvent::Kind::Lost, player.position, false});
    }
    else if (enemy.health <= 0)
    {
        state = State::Won;
        events.push_back({GameEvent::Kind::Won, enemy.position, true});
    }
}
int Game::Score() const
{
    return std::max(
        0, static_cast<int>(hits * 100 + player.health * 10 +
                            (state == State::Won ? std::max(0.f, 1800 - elapsed * 20) : 0)));
}
int Game::Accuracy() const
{
    return shots ? static_cast<int>(std::round(100.f * hits / shots)) : 0;
}
} // namespace skybound
