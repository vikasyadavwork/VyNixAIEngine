#include <vx/Scene/Scene.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace vx
{
namespace
{
constexpr std::size_t MaxEntities = 100000;
constexpr std::size_t MaxCells = 1000000;
constexpr std::size_t MaxString = 4096;
constexpr float Epsilon = 0.00001f;
bool finite(float value)
{
    return std::isfinite(value);
}
bool finite(Vec2 value)
{
    return finite(value.x) && finite(value.y);
}
bool validRect(Rect value)
{
    return finite(value.x) && finite(value.y) && finite(value.w) && finite(value.h) &&
           value.w > 0 && value.h > 0;
}
bool validColor(const SceneColor& value)
{
    return std::all_of(value.begin(), value.end(),
                       [](float c) { return finite(c) && c >= 0 && c <= 1; });
}
bool bounded(float value)
{
    return finite(value) && std::abs(value) <= 1000000000.0f;
}
bool bounded(Vec2 value)
{
    return bounded(value.x) && bounded(value.y);
}
bool boundedRect(Rect value)
{
    return validRect(value) && bounded(value.x) && bounded(value.y) && bounded(value.w) &&
           bounded(value.h);
}
bool validEntity(const Entity& e)
{
    const Rect world = e.worldBounds();
    return e.name.size() <= MaxString && e.sprite.resource.size() <= MaxString &&
           e.script.resource.size() <= MaxString && bounded(e.transform.position) &&
           bounded(e.transform.scale) && bounded(e.transform.rotation) &&
           bounded(e.body.velocity) && bounded(e.body.gravityScale) && finite(e.body.restitution) &&
           e.body.restitution >= 0 && e.body.restitution <= 1 && boundedRect(e.collider.bounds) &&
           (!e.collider.enabled || boundedRect(world)) && boundedRect(e.sprite.frame) &&
           bounded(e.sprite.size) && e.sprite.size.x > 0 && e.sprite.size.y > 0 &&
           validColor(e.sprite.color);
}
void checkDt(float dt)
{
    if (!finite(dt) || dt < 0 || dt > 1)
        throw std::invalid_argument("dt must be finite and in [0, 1]");
}
struct UpdateGuard
{
    bool& flag;
    explicit UpdateGuard(bool& value) : flag(value)
    {
        if (flag)
            throw std::logic_error("update cannot be called recursively");
        flag = true;
    }
    ~UpdateGuard()
    {
        flag = false;
    }
};
float dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}
struct Penetration
{
    Vec2 normal;
    float depth;
};
Penetration penetration(Rect a, Rect b)
{
    const float dx = a.x - b.x, dy = a.y - b.y;
    const float px = (a.w + b.w) * 0.5f - std::abs(dx);
    const float py = (a.h + b.h) * 0.5f - std::abs(dy);
    if (px <= py)
        return {{dx < 0 ? -1.0f : 1.0f, 0}, px};
    return {{0, dy < 0 ? -1.0f : 1.0f}, py};
}
void bounce(RigidBody& body, Vec2 normal)
{
    const float approach = dot(body.velocity, normal);
    if (approach < 0)
        body.velocity = body.velocity - normal * ((1 + body.restitution) * approach);
}
bool readBool(std::istream& in, bool& value)
{
    int parsed = -1;
    if (!(in >> parsed) || (parsed != 0 && parsed != 1))
        return false;
    value = parsed != 0;
    return true;
}
bool tag(std::istream& in, const char* expected)
{
    in >> std::ws;
    for (; *expected; ++expected)
        if (in.get() != *expected)
            return false;
    const int next = in.peek();
    return next == std::char_traits<char>::eof() || std::isspace(static_cast<unsigned char>(next));
}
template <typename T> bool readUnsigned(std::istream& in, T& value)
{
    in >> std::ws;
    std::uint64_t result = 0;
    bool any = false;
    for (int digits = 0; digits < 21; ++digits)
    {
        const int c = in.peek();
        if (c < '0' || c > '9')
        {
            if (!any || (c != std::char_traits<char>::eof() &&
                         !std::isspace(static_cast<unsigned char>(c))))
                return false;
            value = static_cast<T>(result);
            return true;
        }
        const auto digit = static_cast<std::uint64_t>(in.get() - '0');
        if (result > (std::numeric_limits<T>::max() - digit) / 10)
            return false;
        result = result * 10 + digit;
        any = true;
    }
    return false;
}
// Unlike std::quoted extraction, bound memory before accepting untrusted strings.
bool readString(std::istream& in, std::string& value)
{
    in >> std::ws;
    if (in.get() != '"')
        return false;
    value.clear();
    for (std::size_t i = 0; i <= MaxString; ++i)
    {
        int c = in.get();
        if (c == std::char_traits<char>::eof())
            return false;
        if (c == '"')
            return true;
        if (i == MaxString)
            return false;
        if (c == '\\')
        {
            c = in.get();
            if (c != '\\' && c != '"')
                return false;
        }
        if (c < 32 || c == 127)
            return false;
        value.push_back(static_cast<char>(c));
    }
    return false;
}
bool safeString(const std::string& value)
{
    return value.size() <= MaxString && std::none_of(value.begin(), value.end(), [](unsigned char c)
                                                     { return c < 32 || c == 127; });
}
} // namespace

bool intersects(Rect a, Rect b)
{
    return validRect(a) && validRect(b) && std::abs(a.x - b.x) < (a.w + b.w) * 0.5f &&
           std::abs(a.y - b.y) < (a.h + b.h) * 0.5f;
}

std::optional<SweepHit> sweepSegment(Vec2 start, Vec2 end, Rect target)
{
    if (!finite(start) || !finite(end) || !validRect(target))
        return std::nullopt;
    const Vec2 delta = end - start;
    const Vec2 low{target.x - target.w * 0.5f, target.y - target.h * 0.5f};
    const Vec2 high{target.x + target.w * 0.5f, target.y + target.h * 0.5f};
    float nearTime = -std::numeric_limits<float>::infinity();
    float farTime = std::numeric_limits<float>::infinity();
    Vec2 normal{};
    const auto axis =
        [&](float origin, float direction, float minimum, float maximum, Vec2 lowNormal)
    {
        if (direction == 0)
            return origin >= minimum && origin <= maximum;
        float near = (minimum - origin) / direction;
        float far = (maximum - origin) / direction;
        Vec2 hitNormal = lowNormal;
        if (near > far)
        {
            std::swap(near, far);
            hitNormal = lowNormal * -1;
        }
        if (near > nearTime)
        {
            nearTime = near;
            normal = hitNormal;
        }
        farTime = std::min(farTime, far);
        return nearTime <= farTime;
    };
    if (!axis(start.x, delta.x, low.x, high.x, {-1, 0}) ||
        !axis(start.y, delta.y, low.y, high.y, {0, -1}) || farTime < 0 || nearTime > 1)
        return std::nullopt;
    if (nearTime < 0)
    {
        const bool strictlyInside =
            start.x > low.x && start.x < high.x && start.y > low.y && start.y < high.y;
        if (!strictlyInside)
            return std::nullopt; // Boundary motion going away or parallel.
        normal = std::abs(delta.x) >= std::abs(delta.y) ? Vec2{delta.x > 0 ? -1.0f : 1.0f, 0}
                                                        : Vec2{0, delta.y > 0 ? -1.0f : 1.0f};
        if (delta == Vec2{})
            normal = {};
        return SweepHit{0, normal};
    }
    return SweepHit{std::max(0.0f, nearTime), normal};
}

std::optional<SweepHit> sweepAabb(Rect moving, Vec2 displacement, Rect target)
{
    if (!validRect(moving) || !validRect(target) || !finite(displacement))
        return std::nullopt;
    target.w += moving.w;
    target.h += moving.h;
    return sweepSegment({moving.x, moving.y}, Vec2{moving.x, moving.y} + displacement, target);
}

Rect Entity::worldBounds() const
{
    return {transform.position.x + collider.bounds.x * transform.scale.x,
            transform.position.y + collider.bounds.y * transform.scale.y,
            collider.bounds.w * std::abs(transform.scale.x),
            collider.bounds.h * std::abs(transform.scale.y)};
}

TileMap::TileMap(std::uint32_t width, std::uint32_t height, Vec2 tileSize, Vec2 origin)
    : width_(width), height_(height), tileSize_(tileSize), origin_(origin)
{
    const auto count = static_cast<std::uint64_t>(width) * height;
    if (width == 0 || height == 0 || count > MaxCells || !bounded(tileSize) || !bounded(origin) ||
        tileSize.x <= 0 || tileSize.y <= 0 ||
        !bounded(origin.x + tileSize.x * static_cast<float>(width)) ||
        !bounded(origin.y + tileSize.y * static_cast<float>(height)))
        throw std::invalid_argument("invalid tilemap dimensions or geometry");
    cells_.resize(static_cast<std::size_t>(count), 0);
}
bool TileMap::solid(std::uint32_t x, std::uint32_t y) const
{
    if (x >= width_ || y >= height_)
        throw std::out_of_range("tile outside map");
    return cells_[static_cast<std::size_t>(y) * width_ + x] != 0;
}
void TileMap::setSolid(std::uint32_t x, std::uint32_t y, bool value)
{
    if (x >= width_ || y >= height_)
        throw std::out_of_range("tile outside map");
    cells_[static_cast<std::size_t>(y) * width_ + x] = value ? 1 : 0;
}
std::vector<Rect> TileMap::colliders() const
{
    std::vector<Rect> result;
    for (std::uint32_t y = 0; y < height_; ++y)
    {
        for (std::uint32_t x = 0; x < width_;)
        {
            if (!solid(x, y))
            {
                ++x;
                continue;
            }
            const auto begin = x;
            while (x < width_ && solid(x, y))
                ++x;
            const float width = static_cast<float>(x - begin) * tileSize_.x;
            result.push_back({origin_.x + static_cast<float>(begin) * tileSize_.x + width * 0.5f,
                              origin_.y + (static_cast<float>(y) + 0.5f) * tileSize_.y, width,
                              tileSize_.y});
        }
    }
    return result;
}

EntityId Scene::createEntity(std::string name)
{
    if (entities_.size() >= MaxEntities || nextId_ == std::numeric_limits<EntityId>::max())
        throw std::length_error("scene entity capacity exceeded");
    if (!safeString(name))
        throw std::invalid_argument("invalid entity name");
    const auto id = nextId_++;
    Entity entity;
    entity.id = id;
    entity.name = std::move(name);
    entities_.emplace(id, std::move(entity));
    started_.emplace(id, false);
    return id;
}
Entity* Scene::get(EntityId id)
{
    const auto found = entities_.find(id);
    return found == entities_.end() ? nullptr : &found->second;
}
const Entity* Scene::get(EntityId id) const
{
    const auto found = entities_.find(id);
    return found == entities_.end() ? nullptr : &found->second;
}
bool Scene::destroyEntity(EntityId id)
{
    const auto found = entities_.find(id);
    if (found == entities_.end())
        return false;
    auto callback = found->second.script.onDestroy;
    entities_.erase(found);
    started_.erase(id);
    if (callback)
        callback(*this, id); // ID is invalid before callback, preventing recursive destruction.
    return true;
}
void Scene::clear()
{
    const auto ids = entityIds();
    tileMaps_.clear();
    contacts_.clear();
    for (const auto id : ids)
        destroyEntity(id);
}
std::vector<EntityId> Scene::entityIds() const
{
    std::vector<EntityId> ids;
    ids.reserve(entities_.size());
    for (const auto& [id, unused] : entities_)
    {
        (void)unused;
        ids.push_back(id);
    }
    return ids;
}
void Scene::addTileMap(TileMap map)
{
    if (map.width() == 0 || map.height() == 0)
        throw std::invalid_argument("cannot add empty tilemap");
    if (tileMaps_.size() >= 1024)
        throw std::length_error("tilemap count exceeded");
    std::size_t total = map.cells_.size();
    for (const auto& existing : tileMaps_)
        total += existing.cells_.size();
    if (total > MaxCells)
        throw std::length_error("tilemap cell limit exceeded");
    tileMaps_.push_back(std::move(map));
}

void Scene::update(float dt)
{
    checkDt(dt);
    UpdateGuard guard(updating_);
    if (!bounded(gravity))
        throw std::invalid_argument("invalid scene gravity");
    contacts_.clear();
    const auto ids = entityIds();
    for (const auto id : ids)
    {
        Entity* entity = get(id);
        if (!entity || !entity->active)
            continue;
        if (!started_.at(id))
        {
            started_.at(id) = true;
            auto callback = entity->script.onStart;
            if (callback)
                callback(*this, id);
        }
        entity = get(id);
        if (!entity || !entity->active)
            continue;
        auto callback = entity->script.onUpdate;
        if (callback)
            callback(*this, id, dt);
    }
    struct Obstacle
    {
        Rect rect;
        EntityId id;
        bool trigger;
    };
    std::vector<Obstacle> obstacles;
    for (const auto id : ids)
    {
        const auto* entity = get(id);
        if (!entity || !entity->active)
            continue;
        if (!validEntity(*entity))
            throw std::invalid_argument("invalid entity components");
        if (entity->collider.enabled && !entity->body.dynamic)
            obstacles.push_back({entity->worldBounds(), id, entity->collider.trigger});
    }
    for (const auto& map : tileMaps_)
        for (const auto rect : map.colliders())
            obstacles.push_back({rect, InvalidEntity, false});

    for (const auto id : ids)
    {
        auto* entity = get(id);
        if (!entity || !entity->active || !entity->body.dynamic)
            continue;
        auto& body = entity->body;
        if (body.useGravity)
            body.velocity += gravity * (body.gravityScale * dt);
        if (!finite(body.velocity))
            throw std::overflow_error("physics velocity overflow");
        if (!entity->collider.enabled)
        {
            entity->transform.position += body.velocity * dt;
            continue;
        }
        const Rect initial = entity->worldBounds();
        const Vec2 initialMove = body.velocity * dt;
        const auto recordTriggers = [&](Rect start, Vec2 movement)
        {
            for (const auto& obstacle : obstacles)
            {
                if (!(entity->collider.trigger || obstacle.trigger))
                    continue;
                if (!(intersects(start, obstacle.rect) ||
                      sweepAabb(start, movement, obstacle.rect)))
                    continue;
                const bool recorded = std::any_of(contacts_.begin(), contacts_.end(),
                                                  [&](const Contact& contact)
                                                  {
                                                      return contact.trigger &&
                                                             contact.first == id &&
                                                             contact.second == obstacle.id;
                                                  });
                if (!recorded)
                    contacts_.push_back({id, obstacle.id, {}, true});
            }
        };
        if (entity->collider.trigger)
        {
            recordTriggers(initial, initialMove);
            entity->transform.position += initialMove;
            continue;
        }
        recordTriggers(initial, {});
        // Resolve spawning overlaps first. Multiple passes handle adjoining tiles.
        for (int pass = 0; pass < 4; ++pass)
        {
            bool resolved = false;
            for (const auto& obstacle : obstacles)
            {
                if (obstacle.trigger || !intersects(entity->worldBounds(), obstacle.rect))
                    continue;
                const auto hit = penetration(entity->worldBounds(), obstacle.rect);
                entity->transform.position += hit.normal * (hit.depth + Epsilon);
                bounce(body, hit.normal);
                contacts_.push_back({id, obstacle.id, hit.normal, false});
                resolved = true;
            }
            if (!resolved)
                break;
        }
        float remaining = dt;
        for (int iteration = 0; iteration < 8 && remaining > 0; ++iteration)
        {
            const Vec2 movement = body.velocity * remaining;
            if (movement == Vec2{})
                break;
            std::optional<SweepHit> earliest;
            EntityId other = InvalidEntity;
            for (const auto& obstacle : obstacles)
            {
                if (obstacle.trigger)
                    continue;
                const auto hit = sweepAabb(entity->worldBounds(), movement, obstacle.rect);
                if (hit && dot(movement, hit->normal) < 0 &&
                    (!earliest || hit->time < earliest->time))
                {
                    earliest = hit;
                    other = obstacle.id;
                }
            }
            if (!earliest)
            {
                recordTriggers(entity->worldBounds(), movement);
                entity->transform.position += movement;
                break;
            }
            recordTriggers(entity->worldBounds(), movement * earliest->time);
            entity->transform.position += movement * earliest->time + earliest->normal * Epsilon;
            bounce(body, earliest->normal);
            contacts_.push_back({id, other, earliest->normal, false});
            remaining *= 1 - earliest->time;
            // Stop after eight impacts; discard remaining time rather than tunnel.
        }
    }
    // Discrete dynamic pairs have equal mass, with stable ID pair ordering.
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        auto* a = get(ids[i]);
        if (!a || !a->active || !a->body.dynamic || !a->collider.enabled)
            continue;
        for (std::size_t j = i + 1; j < ids.size(); ++j)
        {
            auto* b = get(ids[j]);
            if (!b || !b->active || !b->body.dynamic || !b->collider.enabled ||
                !intersects(a->worldBounds(), b->worldBounds()))
                continue;
            const auto hit = penetration(a->worldBounds(), b->worldBounds());
            const bool trigger = a->collider.trigger || b->collider.trigger;
            contacts_.push_back({ids[i], ids[j], hit.normal, trigger});
            if (trigger)
                continue;
            const Vec2 separation = hit.normal * ((hit.depth + Epsilon) * 0.5f);
            a->transform.position += separation;
            b->transform.position += separation * -1;
            const float approach = dot(a->body.velocity - b->body.velocity, hit.normal);
            if (approach < 0)
            {
                const float restitution = std::min(a->body.restitution, b->body.restitution);
                const Vec2 impulse = hit.normal * (-(1 + restitution) * approach * 0.5f);
                a->body.velocity += impulse;
                b->body.velocity += impulse * -1;
            }
        }
    }
}

bool Scene::save(std::ostream& stream) const
{
    if (!bounded(gravity))
        return false;
    // Build the entire document first, so invalid components cannot emit a partial scene.
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "VXN_SCENE 1\nGRAVITY " << gravity.x << ' ' << gravity.y << "\nNEXT_ID " << nextId_;
    out << "\nENTITIES " << entities_.size() << '\n';
    for (const auto& [id, e] : entities_)
    {
        if (!validEntity(e) || !safeString(e.name) || !safeString(e.sprite.resource) ||
            !safeString(e.script.resource))
            return false;
        out << "ENTITY " << id << ' ' << std::quoted(e.name) << ' ' << e.active << '\n';
        out << "TRANSFORM " << e.transform.position.x << ' ' << e.transform.position.y << ' '
            << e.transform.scale.x << ' ' << e.transform.scale.y << ' ' << e.transform.rotation
            << '\n';
        out << "SPRITE " << std::quoted(e.sprite.resource) << ' ' << e.sprite.frame.x << ' '
            << e.sprite.frame.y << ' ' << e.sprite.frame.w << ' ' << e.sprite.frame.h << ' '
            << e.sprite.size.x << ' ' << e.sprite.size.y;
        for (const auto c : e.sprite.color)
            out << ' ' << c;
        out << ' ' << e.sprite.visible << '\n';
        out << "BODY " << e.body.velocity.x << ' ' << e.body.velocity.y << ' ' << e.body.dynamic
            << ' ' << e.body.useGravity << ' ' << e.body.gravityScale << ' ' << e.body.restitution
            << '\n';
        out << "COLLIDER " << e.collider.bounds.x << ' ' << e.collider.bounds.y << ' '
            << e.collider.bounds.w << ' ' << e.collider.bounds.h << ' ' << e.collider.enabled << ' '
            << e.collider.trigger << '\n';
        out << "SCRIPT " << std::quoted(e.script.resource) << "\nEND_ENTITY\n";
    }
    out << "TILEMAPS " << tileMaps_.size() << '\n';
    for (const auto& map : tileMaps_)
    {
        out << "TILEMAP " << map.width_ << ' ' << map.height_ << ' ' << map.tileSize_.x << ' '
            << map.tileSize_.y << ' ' << map.origin_.x << ' ' << map.origin_.y << "\nCELLS ";
        for (const auto cell : map.cells_)
            out << (cell ? '1' : '0');
        out << "\nEND_TILEMAP\n";
    }
    out << "END_SCENE\n";
    stream << out.str();
    return static_cast<bool>(stream);
}

bool Scene::load(std::istream& stream, std::string* error)
{
    const auto fail = [&](const char* reason)
    {
        if (error)
            *error = reason;
        return false;
    };
    if (updating_)
        return fail("cannot replace a scene during update");
    try
    {
        // Parse into a temporary scene. Do not execute callbacks while loading data.
        Scene parsed;
        struct LocaleGuard
        {
            std::istream& stream;
            std::locale previous;
            explicit LocaleGuard(std::istream& in) : stream(in), previous(in.getloc())
            {
                stream.imbue(std::locale::classic());
            }
            ~LocaleGuard()
            {
                try
                {
                    stream.imbue(previous);
                }
                catch (...)
                {
                }
            }
        } locale(stream);
        int version = 0;
        if (!tag(stream, "VXN_SCENE") || !(stream >> version) || version != 1)
            return fail("unsupported scene version");
        if (!tag(stream, "GRAVITY") || !(stream >> parsed.gravity.x >> parsed.gravity.y) ||
            !bounded(parsed.gravity))
            return fail("invalid gravity");
        if (!tag(stream, "NEXT_ID") || !readUnsigned(stream, parsed.nextId_) || parsed.nextId_ == 0)
            return fail("invalid next entity ID");
        std::uint64_t count = 0;
        if (!tag(stream, "ENTITIES") || !readUnsigned(stream, count) || count > MaxEntities)
            return fail("invalid entity count");
        for (std::uint64_t i = 0; i < count; ++i)
        {
            Entity e;
            if (!tag(stream, "ENTITY") || !readUnsigned(stream, e.id) || e.id == InvalidEntity ||
                e.id >= parsed.nextId_ || parsed.entities_.contains(e.id) ||
                !readString(stream, e.name) || !readBool(stream, e.active))
                return fail("invalid or duplicate entity ID/name");
            if (!tag(stream, "TRANSFORM") ||
                !(stream >> e.transform.position.x >> e.transform.position.y >>
                  e.transform.scale.x >> e.transform.scale.y >> e.transform.rotation))
                return fail("invalid transform");
            if (!tag(stream, "SPRITE") || !readString(stream, e.sprite.resource) ||
                !(stream >> e.sprite.frame.x >> e.sprite.frame.y >> e.sprite.frame.w >>
                  e.sprite.frame.h >> e.sprite.size.x >> e.sprite.size.y))
                return fail("invalid sprite");
            for (auto& c : e.sprite.color)
                if (!(stream >> c))
                    return fail("invalid sprite color");
            if (!readBool(stream, e.sprite.visible))
                return fail("invalid sprite visibility");
            if (!tag(stream, "BODY") || !(stream >> e.body.velocity.x >> e.body.velocity.y) ||
                !readBool(stream, e.body.dynamic) || !readBool(stream, e.body.useGravity) ||
                !(stream >> e.body.gravityScale >> e.body.restitution))
                return fail("invalid rigid body");
            if (!tag(stream, "COLLIDER") ||
                !(stream >> e.collider.bounds.x >> e.collider.bounds.y >> e.collider.bounds.w >>
                  e.collider.bounds.h) ||
                !readBool(stream, e.collider.enabled) || !readBool(stream, e.collider.trigger))
                return fail("invalid collider");
            if (!tag(stream, "SCRIPT") || !readString(stream, e.script.resource) ||
                !tag(stream, "END_ENTITY") || !validEntity(e))
                return fail("invalid entity components");
            parsed.started_.emplace(e.id, false);
            parsed.entities_.emplace(e.id, std::move(e));
        }
        if (!tag(stream, "TILEMAPS") || !readUnsigned(stream, count) || count > 1024)
            return fail("invalid tilemap count");
        std::size_t totalCells = 0;
        for (std::uint64_t i = 0; i < count; ++i)
        {
            std::uint32_t width = 0, height = 0;
            Vec2 tileSize, origin;
            if (!tag(stream, "TILEMAP") || !readUnsigned(stream, width) ||
                !readUnsigned(stream, height) ||
                !(stream >> tileSize.x >> tileSize.y >> origin.x >> origin.y))
                return fail("invalid tilemap geometry");
            const auto cells = static_cast<std::uint64_t>(width) * height;
            if (cells > MaxCells || totalCells + cells > MaxCells)
                return fail("tilemap cell limit exceeded");
            totalCells += static_cast<std::size_t>(cells);
            TileMap map(width, height, tileSize, origin);
            if (!tag(stream, "CELLS"))
                return fail("missing tilemap cells");
            stream >> std::ws;
            for (auto& cell : map.cells_)
            {
                const int value = stream.get();
                if (value != '0' && value != '1')
                    return fail("invalid tilemap cell");
                cell = static_cast<std::uint8_t>(value - '0');
            }
            if (!tag(stream, "END_TILEMAP"))
                return fail("invalid tilemap end");
            parsed.tileMaps_.push_back(std::move(map));
        }
        if (!tag(stream, "END_SCENE"))
            return fail("missing scene end");
        stream >> std::ws;
        if (stream.peek() != std::char_traits<char>::eof())
            return fail("unexpected data after scene");
        gravity = parsed.gravity;
        entities_.swap(parsed.entities_);
        started_.swap(parsed.started_);
        tileMaps_.swap(parsed.tileMaps_);
        contacts_.clear();
        nextId_ = parsed.nextId_;
        if (error)
            error->clear();
        return true;
    }
    catch (const std::exception& ex)
    {
        if (error)
            *error = ex.what();
        return false;
    }
}

bool SceneManager::add(std::string name, std::unique_ptr<Scene> scene)
{
    if (name.empty() || !scene || scenes_.contains(name))
        return false;
    scenes_.emplace(std::move(name), std::shared_ptr<Scene>(std::move(scene)));
    return true;
}
bool SceneManager::activate(const std::string& name)
{
    if (!scenes_.contains(name))
        return false;
    activeName_ = name;
    return true;
}
bool SceneManager::remove(const std::string& name)
{
    const auto found = scenes_.find(name);
    if (found == scenes_.end())
        return false;
    auto scene = std::move(found->second);
    scenes_.erase(found);
    if (activeName_ == name)
        activeName_.clear();
    scene->clear();
    return true;
}
Scene* SceneManager::active()
{
    return find(activeName_);
}
const Scene* SceneManager::active() const
{
    const auto found = scenes_.find(activeName_);
    return found == scenes_.end() ? nullptr : found->second.get();
}
Scene* SceneManager::find(const std::string& name)
{
    const auto found = scenes_.find(name);
    return found == scenes_.end() ? nullptr : found->second.get();
}
void SceneManager::update(float dt)
{
    const auto found = scenes_.find(activeName_);
    if (found == scenes_.end())
        return;
    // A callback may remove its own scene; keep the update receiver alive.
    auto scene = found->second;
    scene->update(dt);
}

std::size_t Animation::frameIndex(float elapsed) const
{
    if (frames.empty())
        return 0;
    if (!finite(elapsed) || elapsed < 0 || !finite(framesPerSecond) || framesPerSecond <= 0)
        throw std::invalid_argument("invalid animation timing");
    const double frameNumber = std::floor(static_cast<double>(elapsed) * framesPerSecond);
    if (!loop)
        return static_cast<std::size_t>(
            std::min(frameNumber, static_cast<double>(frames.size() - 1)));
    return static_cast<std::size_t>(std::fmod(frameNumber, static_cast<double>(frames.size())));
}
Rect Animation::frame(float elapsed) const
{
    if (frames.empty())
        return {0, 0, 1, 1};
    return frames[frameIndex(elapsed)];
}
Tween::Tween(Vec2 from, Vec2 to, float duration, Easing easing)
    : from_(from), to_(to), duration_(duration), easing_(easing)
{
    if (!finite(from) || !finite(to) || !finite(duration) || duration < 0)
        throw std::invalid_argument("invalid tween");
}
Vec2 Tween::advance(float dt)
{
    checkDt(dt);
    elapsed_ = std::min(duration_, elapsed_ + dt);
    return value();
}
Vec2 Tween::value() const
{
    float t = duration_ == 0 ? 1 : elapsed_ / duration_;
    if (easing_ == Easing::Smoothstep)
        t = t * t * (3 - 2 * t);
    else if (easing_ == Easing::EaseInOut)
        t = t < 0.5f ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2.0f) * 0.5f;
    return from_ * (1 - t) + to_ * t;
}
TimerQueue::TimerId TimerQueue::after(float delay, std::function<void()> callback)
{
    if (!finite(delay) || delay < 0 || !callback)
        throw std::invalid_argument("invalid timer");
    if (nextId_ == std::numeric_limits<TimerId>::max())
        throw std::length_error("timer ID exhausted");
    const auto id = nextId_++;
    timers_.emplace(id, Timer{delay, 0, std::move(callback)});
    return id;
}
TimerQueue::TimerId TimerQueue::every(float interval, std::function<void()> callback)
{
    if (!finite(interval) || interval <= 0)
        throw std::invalid_argument("timer interval must be positive");
    const auto id = after(interval, std::move(callback));
    timers_.at(id).interval = interval;
    return id;
}
bool TimerQueue::cancel(TimerId id)
{
    return timers_.erase(id) != 0;
}
void TimerQueue::update(float dt)
{
    checkDt(dt);
    UpdateGuard guard(updating_);
    std::vector<TimerId> ids;
    for (const auto& [id, unused] : timers_)
    {
        (void)unused;
        ids.push_back(id);
    }
    for (const auto id : ids)
    {
        const auto found = timers_.find(id);
        if (found == timers_.end())
            continue;
        auto& timer = found->second;
        timer.remaining -= dt;
        if (timer.remaining > 0)
            continue;
        auto callback = timer.callback;
        if (timer.interval == 0)
            timers_.erase(found);
        else
        {
            timer.remaining = std::fmod(timer.remaining, timer.interval) + timer.interval;
        }
        callback();
    }
}
ParticleEmitter::ParticleEmitter(std::size_t capacity, std::uint32_t seed)
    : capacity_(capacity), random_(seed)
{
    if (capacity > MaxCells)
        throw std::length_error("particle capacity exceeded");
    particles_.reserve(capacity);
}
float ParticleEmitter::randomSigned()
{
    random_ = random_ * 1664525u + 1013904223u;
    return static_cast<float>(random_ >> 8) / 8388607.5f - 1;
}
std::size_t ParticleEmitter::emit(std::size_t count, const ParticleSettings& settings)
{
    if (!finite(settings.position) || !finite(settings.velocity) ||
        !finite(settings.velocitySpread) || settings.velocitySpread.x < 0 ||
        settings.velocitySpread.y < 0 || !finite(settings.lifetime) || settings.lifetime <= 0 ||
        !finite(settings.size) || settings.size <= 0 || !validColor(settings.color))
        throw std::invalid_argument("invalid particle settings");
    count = std::min(count, capacity_ - particles_.size());
    for (std::size_t i = 0; i < count; ++i)
    {
        Particle particle;
        particle.position = settings.position;
        particle.velocity = settings.velocity + Vec2{randomSigned() * settings.velocitySpread.x,
                                                     randomSigned() * settings.velocitySpread.y};
        particle.lifetime = settings.lifetime;
        particle.size = settings.size;
        particle.color = settings.color;
        particles_.push_back(particle);
    }
    return count;
}
void ParticleEmitter::update(float dt, Vec2 acceleration)
{
    checkDt(dt);
    if (!finite(acceleration))
        throw std::invalid_argument("invalid particle acceleration");
    for (auto& particle : particles_)
    {
        particle.age += dt;
        particle.velocity += acceleration * dt;
        particle.position += particle.velocity * dt;
    }
    std::erase_if(particles_, [](const Particle& p) { return p.age >= p.lifetime; });
}

} // namespace vx
