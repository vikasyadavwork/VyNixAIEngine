#include "SkyboundGame.h"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace skybound;
static void Check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main()
{
    try
    {
        Game game;
        game.Start();
        Check(game.player.health == 100 && game.enemy.health == 100,
              "Both planes must start with 100 health");
        game.Update(1.f / 120, {{1, 1}, true});
        Check(game.shots == 1 && game.bullets.size() == 1, "Player fire should create projectile");
        const auto position = game.player.position;
        const auto bullets = game.bullets.size();
        game.Pause();
        game.Update(1.f / 120, {{1, 0}, true});
        Check(game.player.position == position && game.bullets.size() == bullets,
              "Pause freezes simulation");
        game.Pause();
        game.Start();
        for (int i = 0; i < 1200; i++)
            game.Update(1.f / 120, {{-1, -1}, false});
        Check(game.player.position.x >= 65 && game.player.position.y >= 100,
              "Plane must remain in player airspace");
        game.Start();
        game.bullets.push_back(
            {{game.enemy.position.x - 45, game.enemy.position.y}, {20000, 0}, true});
        game.Update(1.f / 120, {});
        Check(game.enemy.health == 90, "Swept projectile must hit once for ten damage");
        game.Start();
        game.enemy.health = 10;
        game.bullets.push_back(
            {{game.enemy.position.x - 45, game.enemy.position.y}, {20000, 0}, true});
        game.Update(1.f / 120, {});
        Check(game.state == State::Won && game.enemy.health == 0, "Fatal enemy hit must win");
        const auto score = game.Score();
        game.Update(.05f, {{}, true});
        Check(game.Score() == score, "Results freeze state");
        game.Start(Difficulty::Ace);
        game.player.health = 10;
        game.bullets.push_back(
            {{game.player.position.x + 45, game.player.position.y}, {-20000, 0}, false});
        game.Update(1.f / 120, {});
        Check(game.state == State::Lost && game.player.health == 0, "Fatal player hit must lose");
        game.Start();
        Check(game.hits == 0 && game.shots == 0 && game.bullets.empty() && game.elapsed == 0,
              "Restart clears mission state");
        game.Update(std::numeric_limits<float>::quiet_NaN(), {});
        Check(game.elapsed == 0, "Reject invalid timestep");
        game.Start();
        for (int i = 0; i < 600; i++)
            game.Update(1.f / 120, {});
        Check(game.player.health < 100, "AI must shoot and damage player");
        game.Start();
        for (int i = 0; i < 160; i++)
            game.Update(1.f / 120, {{}, true});
        Check(game.shots >= 8 && game.shots <= 9, "Machine gun cooldown must be rate limited");
        Check(game.Accuracy() >= 0 && game.Accuracy() <= 100, "Accuracy bounds");
        std::cout << "Skybound: 12 gameplay checks passed\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
