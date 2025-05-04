#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace vx
{

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
    constexpr Vec2 operator+(Vec2 v) const
    {
        return {x + v.x, y + v.y};
    }
    constexpr Vec2 operator-(Vec2 v) const
    {
        return {x - v.x, y - v.y};
    }
    constexpr Vec2 operator*(float s) const
    {
        return {x * s, y * s};
    }
    constexpr Vec2 operator/(float s) const
    {
        return {x / s, y / s};
    }
    constexpr Vec2& operator+=(Vec2 v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }
    constexpr bool operator==(const Vec2&) const = default;
};

// Centered, axis-aligned rectangle. Positive dimensions are required for collision.
struct Rect
{
    float x = 0, y = 0, w = 1, h = 1;
};
struct SweepHit
{
    float time = 0;
    Vec2 normal{};
};
bool intersects(Rect a, Rect b);
std::optional<SweepHit> sweepSegment(Vec2 start, Vec2 end, Rect target);
std::optional<SweepHit> sweepAabb(Rect moving, Vec2 displacement, Rect target);

using EntityId = std::uint64_t;
inline constexpr EntityId InvalidEntity = 0;
using SceneColor = std::array<float, 4>;
class Scene;

struct Transform
{
    Vec2 position{};
    Vec2 scale{1, 1};
    float rotation = 0; // Radians; visual only, colliders remain axis aligned.
};
struct Sprite
{
    std::string resource; // Asset path, interpreted by the application renderer.
    Rect frame{0, 0, 1, 1};
    Vec2 size{1, 1};
    SceneColor color{1, 1, 1, 1};
    bool visible = true;
};
struct RigidBody
{
    Vec2 velocity{};
    bool dynamic = true;
    bool useGravity = false;
    float gravityScale = 1;
    float restitution = 0;
};
struct Collider
{
    Rect bounds{0, 0, 1, 1}; // Offset and size in local space.
    bool enabled = false;
    bool trigger = false; // Reports contacts without resolving motion.
};
struct Script
{
    std::string resource; // Optional application binding key. No runtime code is loaded.
    std::function<void(Scene&, EntityId)> onStart;
    std::function<void(Scene&, EntityId, float)> onUpdate;
    std::function<void(Scene&, EntityId)> onDestroy;
};
struct Entity
{
    EntityId id = InvalidEntity; // Assigned by Scene; do not change.
    std::string name;
    bool active = true;
    Transform transform;
    Sprite sprite;
    RigidBody body;
    Collider collider;
    Script script;
    Rect worldBounds() const;
};
struct Contact
{
    EntityId first = InvalidEntity;
    EntityId second = InvalidEntity; // InvalidEntity means a tilemap wall.
    Vec2 normal{};                   // Points from second toward first.
    bool trigger = false;
};

class TileMap
{
  public:
    TileMap() = default;
    TileMap(std::uint32_t width, std::uint32_t height, Vec2 tileSize = {1, 1}, Vec2 origin = {});
    std::uint32_t width() const
    {
        return width_;
    }
    std::uint32_t height() const
    {
        return height_;
    }
    Vec2 tileSize() const
    {
        return tileSize_;
    }
    Vec2 origin() const
    {
        return origin_;
    } // Top-left corner, positive y points down.
    bool solid(std::uint32_t x, std::uint32_t y) const;
    void setSolid(std::uint32_t x, std::uint32_t y, bool value = true);
    std::vector<Rect> colliders() const; // Adjacent cells are merged within each row.
  private:
    std::uint32_t width_ = 0, height_ = 0;
    Vec2 tileSize_{1, 1}, origin_{};
    std::vector<std::uint8_t> cells_;
    friend class Scene;
};

class Scene
{
  public:
    Vec2 gravity{0, 980};
    EntityId createEntity(std::string name = "Entity");
    Entity* get(EntityId id);
    const Entity* get(EntityId id) const;
    bool destroyEntity(EntityId id);
    // Destroys the current entity snapshot. Entities spawned by onDestroy survive.
    void clear();
    std::vector<EntityId> entityIds() const;
    std::size_t entityCount() const
    {
        return entities_.size();
    }
    void addTileMap(TileMap map);
    const std::vector<TileMap>& tileMaps() const
    {
        return tileMaps_;
    }
    const std::vector<Contact>& contacts() const
    {
        return contacts_;
    }

    // Deterministic insertion-ID order. dt must be finite in [0, 1].
    // Dynamic/static collision is swept; dynamic/dynamic collision is discrete,
    // equal-mass AABB response. Use a fixed timestep for repeatable game behavior.
    // Entities created by callbacks begin on the next update. No angular physics.
    void update(float dt);

    // Version 1 portable text. Callbacks are intentionally not serialized.
    // A failed load leaves this scene unchanged, including existing callbacks.
    bool save(std::ostream& stream) const;
    bool load(std::istream& stream, std::string* error = nullptr);

  private:
    std::map<EntityId, Entity> entities_;
    std::map<EntityId, bool> started_;
    std::vector<TileMap> tileMaps_;
    std::vector<Contact> contacts_;
    EntityId nextId_ = 1;
    bool updating_ = false;
};

class SceneManager
{
  public:
    bool add(std::string name, std::unique_ptr<Scene> scene);
    bool activate(const std::string& name);
    bool remove(const std::string& name);
    Scene* active();
    const Scene* active() const;
    Scene* find(const std::string& name);
    void update(float dt);

  private:
    std::map<std::string, std::shared_ptr<Scene>> scenes_;
    std::string activeName_;
};

struct Animation
{
    std::vector<Rect> frames;
    float framesPerSecond = 12;
    bool loop = true;
    std::size_t frameIndex(float elapsed) const;
    Rect frame(float elapsed) const;
};

enum class Easing
{
    Linear,
    Smoothstep,
    EaseInOut
};
class Tween
{
  public:
    Tween(Vec2 from, Vec2 to, float duration, Easing easing = Easing::Linear);
    Vec2 advance(float dt);
    Vec2 value() const;
    bool finished() const
    {
        return elapsed_ >= duration_;
    }
    void reset()
    {
        elapsed_ = 0;
    }

  private:
    Vec2 from_, to_;
    float duration_, elapsed_ = 0;
    Easing easing_;
};

class TimerQueue
{
  public:
    using TimerId = std::uint64_t;
    TimerId after(float delay, std::function<void()> callback);
    TimerId every(float interval, std::function<void()> callback);
    bool cancel(TimerId id);
    void clear()
    {
        timers_.clear();
    }
    std::size_t size() const
    {
        return timers_.size();
    }
    // Repeating timers fire at most once per update and retain their phase.
    // Timers created in a callback are first considered on the next update.
    void update(float dt);

  private:
    struct Timer
    {
        double remaining, interval;
        std::function<void()> callback;
    };
    std::map<TimerId, Timer> timers_;
    TimerId nextId_ = 1;
    bool updating_ = false;
};

struct Particle
{
    Vec2 position{}, velocity{};
    float age = 0, lifetime = 1, size = 1;
    SceneColor color{1, 1, 1, 1};
};
struct ParticleSettings
{
    Vec2 position{}, velocity{}, velocitySpread{};
    float lifetime = 1, size = 1;
    SceneColor color{1, 1, 1, 1};
};
class ParticleEmitter
{
  public:
    explicit ParticleEmitter(std::size_t capacity = 256, std::uint32_t seed = 1);
    std::size_t emit(std::size_t count, const ParticleSettings& settings);
    void update(float dt, Vec2 acceleration = {});
    void clear()
    {
        particles_.clear();
    }
    std::size_t capacity() const
    {
        return capacity_;
    }
    const std::vector<Particle>& particles() const
    {
        return particles_;
    }

  private:
    float randomSigned();
    std::size_t capacity_;
    std::uint32_t random_;
    std::vector<Particle> particles_;
};

} // namespace vx
