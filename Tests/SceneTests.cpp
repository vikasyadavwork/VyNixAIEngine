#include <vx/Scene/Scene.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
bool close(float a, float b, float tolerance = 0.001f)
{
    return std::abs(a - b) < tolerance;
}
template <typename Callback> void mustThrow(Callback callback, const char* message)
{
    bool threw = false;
    try
    {
        callback();
    }
    catch (const std::exception&)
    {
        threw = true;
    }
    require(threw, message);
}
std::string serialize(const vx::Scene& scene)
{
    std::ostringstream out;
    require(scene.save(out), "scene save failed");
    return out.str();
}
void replace(std::string& text, const std::string& from, const std::string& to)
{
    const auto at = text.find(from);
    require(at != std::string::npos, "test replacement failed");
    text.replace(at, from.size(), to);
}

void geometry()
{
    require(vx::intersects({0, 0, 2, 2}, {1, 0, 2, 2}), "AABB overlap missed");
    require(!vx::intersects({0, 0, 2, 2}, {2, 0, 2, 2}), "touching is not penetration");
    require(!vx::intersects({0, 0, -2, 2}, {0, 0, 2, 2}), "invalid AABB accepted");
    auto hit = vx::sweepSegment({0, 0}, {100, 0}, {50, 0, 2, 4});
    require(hit && close(hit->time, 0.49f) && hit->normal == vx::Vec2{-1, 0},
            "fast segment tunneled");
    require(!vx::sweepSegment({0, 4}, {100, 4}, {50, 0, 2, 4}), "parallel segment false hit");
    hit = vx::sweepAabb({0, 0, 2, 2}, {100, 0}, {50, 0, 2, 4});
    require(hit && close(hit->time, 0.48f), "swept box extent ignored");
    require(!vx::sweepAabb({48, 0, 2, 2}, {-10, 0}, {50, 0, 2, 4}),
            "moving away stuck on boundary");
    require(!vx::sweepAabb({48, 0, 2, 2}, {0, 10}, {50, 0, 2, 4}),
            "tangent motion stuck on boundary");
    require(vx::sweepSegment({50, 0}, {50, 0}, {50, 0, 2, 4}).has_value(),
            "stationary inside point missed");
    require(!vx::sweepSegment({NAN, 0}, {10, 0}, {5, 0, 2, 2}), "nonfinite geometry accepted");
}

void collision()
{
    vx::Scene scene;
    const auto wall = scene.createEntity("Wall");
    scene.get(wall)->body.dynamic = false;
    scene.get(wall)->collider.enabled = true;
    scene.get(wall)->collider.bounds = {0, 0, 1, 100};
    scene.get(wall)->transform.position = {5, 0};
    const auto mover = scene.createEntity("Mover");
    auto* entity = scene.get(mover);
    entity->collider.enabled = true;
    entity->body.velocity = {100, 2};
    scene.update(0.1f);
    require(close(entity->transform.position.x, 4), "dynamic body tunneled through wall");
    require(close(entity->transform.position.y, 0.2f), "wall sliding lost tangential movement");
    require(close(entity->body.velocity.x, 0), "inelastic collision did not stop velocity");
    require(!scene.contacts().empty() && scene.contacts()[0].second == wall,
            "static contact missing");
    const auto hiddenTrigger = scene.createEntity("Trigger behind wall");
    scene.get(hiddenTrigger)->transform.position = {8, 0};
    scene.get(hiddenTrigger)->body.dynamic = false;
    scene.get(hiddenTrigger)->collider.enabled = true;
    scene.get(hiddenTrigger)->collider.trigger = true;
    entity->transform.position = {0, 0};
    entity->body.velocity = {100, 0};
    scene.update(0.1f);
    require(scene.contacts().size() == 1 && scene.contacts()[0].second == wall,
            "trigger fired beyond blocking wall");
    scene.destroyEntity(hiddenTrigger);
    entity->transform.position = {0, 0};
    entity->body.velocity = {100, 0};
    entity->body.restitution = 1;
    scene.update(0.1f);
    require(close(entity->body.velocity.x, -100) && close(entity->transform.position.x, -2),
            "elastic swept bounce lost remaining time");
    entity->transform.position = {5, 0};
    entity->body.velocity = {};
    scene.update(0);
    require(!vx::intersects(entity->worldBounds(), scene.get(wall)->worldBounds()),
            "spawn penetration unresolved");
    entity->transform.position = {0, 0};
    entity->body.velocity = {100, 0};
    entity->collider.trigger = true;
    scene.update(0.1f);
    require(close(entity->transform.position.x, 10) && scene.contacts()[0].trigger,
            "trigger blocked or missed motion");

    vx::Scene falling;
    vx::TileMap tiles(4, 2, {10, 10});
    tiles.setSolid(0, 1);
    tiles.setSolid(1, 1);
    tiles.setSolid(2, 1);
    require(tiles.colliders().size() == 1, "tile row colliders not merged");
    falling.addTileMap(tiles);
    const auto ball = falling.createEntity();
    falling.get(ball)->transform.position = {5, -20};
    falling.get(ball)->body.velocity = {0, 1000};
    falling.get(ball)->collider.enabled = true;
    falling.update(0.1f);
    require(close(falling.get(ball)->transform.position.y, 9.5f),
            "body tunneled through tile floor");
    require(falling.contacts()[0].second == vx::InvalidEntity, "tile contact ID wrong");
    mustThrow([&] { tiles.setSolid(4, 0); }, "tile bounds not checked");

    vx::Scene pair;
    const auto left = pair.createEntity(), right = pair.createEntity();
    for (const auto id : {left, right})
    {
        pair.get(id)->collider.enabled = true;
        pair.get(id)->collider.bounds = {0, 0, 2, 2};
        pair.get(id)->body.restitution = 1;
    }
    pair.get(left)->transform.position = {-1, 0};
    pair.get(left)->body.velocity = {2, 0};
    pair.get(right)->transform.position = {1, 0};
    pair.get(right)->body.velocity = {-2, 0};
    pair.update(0.25f);
    require(close(pair.get(left)->body.velocity.x, -2) &&
                close(pair.get(right)->body.velocity.x, 2),
            "equal-mass impulse incorrect");
    require(!vx::intersects(pair.get(left)->worldBounds(), pair.get(right)->worldBounds()),
            "dynamic pair remains overlapped");
    mustThrow([&] { pair.update(-0.1f); }, "negative dt accepted");
    mustThrow([&] { pair.update(NAN); }, "nonfinite dt accepted");
}

void serialization()
{
    vx::Scene original;
    original.gravity = {1, 12};
    const auto id = original.createEntity("Pilot \"one\" \\ path");
    auto* entity = original.get(id);
    entity->transform.position = {12.34567f, -6};
    entity->transform.scale = {-2, 3};
    entity->sprite.resource = "assets/plane.png";
    entity->sprite.color = {0.2f, 0.4f, 0.8f, 1};
    entity->script.resource = "player-controller";
    entity->collider.enabled = true;
    entity->body.useGravity = true;
    entity->body.restitution = 0.25f;
    vx::TileMap map(3, 2, {8, 9}, {-1, 4});
    map.setSolid(1, 1);
    original.addTileMap(map);
    const auto saved = serialize(original);
    vx::Scene loaded;
    std::istringstream good(saved);
    std::string error;
    require(loaded.load(good, &error) && error.empty(), "round-trip load failed");
    require(serialize(loaded) == saved, "round-trip scene changed values");
    require(loaded.get(id)->script.resource == "player-controller" &&
                !loaded.get(id)->script.onUpdate,
            "script resource binding invalid");
    require(loaded.createEntity() == id + 1, "next entity ID not restored");

    vx::Scene destination;
    const auto retained = destination.createEntity("Retained");
    int updates = 0;
    destination.get(retained)->script.onUpdate = [&](vx::Scene&, vx::EntityId, float)
    { ++updates; };
    const auto before = serialize(destination);
    const auto rejects = [&](const std::string& input)
    {
        std::istringstream bad(input);
        require(!destination.load(bad, &error) && !error.empty(), "malformed scene accepted");
        require(serialize(destination) == before, "failed load changed existing scene");
    };
    rejects(saved.substr(0, saved.size() / 2));
    rejects(saved + "unexpected");
    auto bad = saved;
    replace(bad, "VXN_SCENE 1", "VXN_SCENE 99");
    rejects(bad);
    bad = saved;
    replace(bad, "ENTITIES 1", "ENTITIES 100001");
    rejects(bad);
    bad = saved;
    replace(bad, "NEXT_ID 2", "NEXT_ID -2");
    rejects(bad);
    bad = saved;
    replace(bad, "ENTITY 1", "ENTITY 2");
    rejects(bad);
    bad = saved;
    replace(bad, "GRAVITY 1 12", "GRAVITY nan 12");
    rejects(bad);
    bad = saved;
    replace(bad, "COLLIDER 0 0 1 1 1 0", "COLLIDER 0 0 -1 1 1 0");
    rejects(bad);
    bad = saved;
    replace(bad, "COLLIDER 0 0 1 1 1 0", "COLLIDER 0 0 1 1 2 0");
    rejects(bad);
    bad = saved;
    replace(bad, "CELLS 000010", "CELLS 000x10");
    rejects(bad);
    bad = saved;
    replace(bad, "\"assets/plane.png\"", "\"" + std::string(4097, 'a') + "\"");
    rejects(bad);
    bad = saved;
    const auto begin = bad.find("ENTITY 1"),
               end = bad.find("END_ENTITY\n") + std::string("END_ENTITY\n").size();
    const auto copy = bad.substr(begin, end - begin);
    bad.insert(end, copy);
    replace(bad, "ENTITIES 1", "ENTITIES 2");
    rejects(bad);
    destination.update(0);
    require(updates == 1, "failed load lost callback");
    destination.get(retained)->body.restitution = 2;
    std::ostringstream invalidOut;
    require(!destination.save(invalidOut) && invalidOut.str().empty(),
            "invalid scene emitted partial document");
}

void lifecycle()
{
    vx::Scene scene;
    int starts = 0, updates = 0, destroys = 0, childUpdates = 0;
    vx::EntityId child = 0;
    const auto parent = scene.createEntity();
    scene.get(parent)->script.onStart = [&](vx::Scene& s, vx::EntityId)
    {
        ++starts;
        child = s.createEntity("Child");
        s.get(child)->body.velocity = {10, 0};
        s.get(child)->script.onUpdate = [&](vx::Scene&, vx::EntityId, float) { ++childUpdates; };
    };
    scene.get(parent)->script.onUpdate = [&](vx::Scene&, vx::EntityId, float) { ++updates; };
    scene.get(parent)->script.onDestroy = [&](vx::Scene& s, vx::EntityId id)
    {
        ++destroys;
        require(s.get(id) == nullptr && !s.destroyEntity(id), "destroy callback saw live ID");
    };
    scene.update(0.1f);
    require(starts == 1 && updates == 1 && childUpdates == 0 &&
                scene.get(child)->transform.position.x == 0,
            "new entities updated in same tick");
    scene.update(0.1f);
    require(starts == 1 && updates == 2 && childUpdates == 1, "entity lifecycle counts incorrect");
    scene.get(parent)->active = false;
    scene.update(0);
    require(updates == 2, "inactive script ran");
    require(scene.destroyEntity(parent) && destroys == 1, "destroy lifecycle failed");
    const auto self = scene.createEntity();
    scene.get(self)->script.onStart = [](vx::Scene& s, vx::EntityId id) { s.destroyEntity(id); };
    scene.update(0);
    require(!scene.get(self), "self deletion in start failed");
    scene.get(child)->script.onUpdate = [](vx::Scene& s, vx::EntityId, float) { s.update(0); };
    mustThrow([&] { scene.update(0); }, "recursive scene update accepted");
    scene.get(child)->script.onUpdate = {};
    scene.update(0);
    scene.clear();
    require(scene.entityCount() == 0, "scene clear failed");

    vx::SceneManager manager;
    auto managed = std::make_unique<vx::Scene>();
    const auto managedId = managed->createEntity();
    managed->get(managedId)->script.onUpdate = [&](vx::Scene&, vx::EntityId, float)
    { manager.remove("game"); };
    require(manager.add("game", std::move(managed)) && manager.activate("game"),
            "scene activation failed");
    require(!manager.activate("missing") && manager.active(), "missing scene changed active scene");
    manager.update(0);
    require(manager.active() == nullptr && manager.find("game") == nullptr,
            "self-removing scene remains active");
}

void animationTimersParticles()
{
    vx::Animation animation{{{0, 0, 8, 8}, {8, 0, 8, 8}, {16, 0, 8, 8}}, 4, true};
    require(animation.frameIndex(0.5f) == 2 && animation.frameIndex(0.75f) == 0,
            "animation looping wrong");
    animation.loop = false;
    require(animation.frameIndex(1000000) == 2, "animation final frame not held");
    vx::Tween tween({0, 0}, {10, 20}, 1, vx::Easing::Smoothstep);
    require(tween.advance(0.5f) == vx::Vec2{5, 10} && !tween.finished(), "tween midpoint wrong");
    require(tween.advance(1) == vx::Vec2{10, 20} && tween.finished(), "tween endpoint wrong");
    vx::Tween instant({0, 0}, {1, 2}, 0);
    require(instant.value() == vx::Vec2{1, 2} && instant.finished(), "zero-duration tween failed");
    vx::TimerQueue timers;
    int once = 0, repeated = 0, created = 0;
    timers.after(0.125f,
                 [&]
                 {
                     ++once;
                     timers.after(0, [&] { ++created; });
                 });
    const auto repeating = timers.every(0.125f, [&] { ++repeated; });
    timers.update(0.125f);
    require(once == 1 && repeated == 1 && created == 0, "timer firing order wrong");
    timers.update(0.5f);
    require(once == 1 && repeated == 2 && created == 1, "timer catch-up bound or creation failed");
    require(timers.cancel(repeating) && !timers.cancel(repeating), "timer cancellation failed");
    vx::TimerQueue::TimerId self = 0;
    self = timers.every(0.125f, [&] { timers.cancel(self); });
    timers.update(0.125f);
    require(timers.size() == 0, "self-cancel timer failed");
    mustThrow([&] { timers.every(0, [] {}); }, "zero repeating interval accepted");

    vx::ParticleEmitter particles(3, 42), repeat(3, 42);
    vx::ParticleSettings settings;
    settings.lifetime = 0.5f;
    settings.velocity = {10, 0};
    settings.velocitySpread = {2, 3};
    require(particles.emit(10, settings) == 3 && particles.emit(1, settings) == 0,
            "particle capacity exceeded");
    repeat.emit(3, settings);
    for (std::size_t i = 0; i < 3; ++i)
        require(particles.particles()[i].velocity == repeat.particles()[i].velocity,
                "particle seed not deterministic");
    particles.update(0.25f, {0, 4});
    require(particles.particles().size() == 3 && particles.particles()[0].position.x > 0,
            "particles did not advance");
    particles.update(0.25f);
    require(particles.particles().empty() && particles.emit(2, settings) == 2,
            "expired particle capacity not reclaimed");
}
} // namespace

int main()
{
    try
    {
        geometry();
        collision();
        serialization();
        lifecycle();
        animationTimersParticles();
        std::cout << "Scene tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Scene test failure: " << error.what() << '\n';
        return 1;
    }
}
