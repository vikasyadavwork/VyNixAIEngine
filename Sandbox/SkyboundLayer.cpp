#include "SkyboundLayer.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vx/Audio/Audio.h>
#include <vx/Core/Input.h>
#ifdef DrawText
#undef DrawText
#endif

namespace
{
constexpr vx::Color Ink{.032f, .047f, .064f, 1}, Panel{.055f, .075f, .094f, 1},
    Line{.13f, .18f, .22f, 1};
constexpr vx::Color Muted{.48f, .59f, .65f, 1}, White{.89f, .94f, .96f, 1},
    Cyan{.26f, .83f, .94f, 1}, Orange{1, .39f, .19f, 1};
constexpr float GameX = 24, GameY = 148, GameW = 900, GameH = 506;
constexpr float StudioX = 236, StudioY = 158, StudioW = 758, StudioH = 426;
float GX(float x)
{
    return GameX + x * GameW / 1200;
}
float GY(float y)
{
    return GameY + y * GameH / 675;
}
std::string Clock(float value)
{
    int n = static_cast<int>(value);
    std::ostringstream s;
    s << std::setfill('0') << std::setw(2) << n / 60 << ':' << std::setw(2) << n % 60;
    return s.str();
}
std::string Number(float value)
{
    return std::to_string(static_cast<int>(std::round(value)));
}
vx::Vec2 MoveAxis()
{
    float x = static_cast<float>(vx::Input::IsKeyDown('D') || vx::Input::IsKeyDown(VK_RIGHT)) -
              static_cast<float>(vx::Input::IsKeyDown('A') || vx::Input::IsKeyDown(VK_LEFT));
    float y = static_cast<float>(vx::Input::IsKeyDown('S') || vx::Input::IsKeyDown(VK_DOWN)) -
              static_cast<float>(vx::Input::IsKeyDown('W') || vx::Input::IsKeyDown(VK_UP));
    return {x, y};
}
std::filesystem::path ExecutableDirectory()
{
    wchar_t path[32768];
    const DWORD n = GetModuleFileNameW(nullptr, path, 32768);
    if (!n || n >= 32768)
        throw std::runtime_error("Cannot locate executable assets.");
    return std::filesystem::path(std::wstring(path, n)).parent_path();
}
} // namespace

SkyboundLayer::SkyboundLayer(vx::Application& app, LaunchOptions options)
    : Layer("Skybound / Vynix Studio"), m_App(app), m_Options(std::move(options))
{
}
std::filesystem::path SkyboundLayer::DataDirectory() const
{
    wchar_t path[32768];
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", path, 32768);
    auto result = n && n < 32768 ? std::filesystem::path(std::wstring(path, n)) / "VyNixAIEngine"
                                 : ExecutableDirectory() / "UserData";
    std::filesystem::create_directories(result);
    return result;
}
void SkyboundLayer::OnAttach()
{
    if (!m_Renderer.Initialize())
        throw std::runtime_error(m_Renderer.GetLastError());
    m_Initialized = true;
    const auto assets = ExecutableDirectory() / "Assets" / "Textures";
    m_Jet = m_Renderer.LoadTexture(assets / "jet.png");
    m_Ocean = m_Renderer.LoadTexture(assets / "ocean.png");
    if (!m_Jet || !m_Ocean)
        throw std::runtime_error(
            "Required aircraft/ocean textures are missing beside the game executable. " +
            m_Renderer.GetLastError());
    m_AudioReady = vx::Audio::Initialize();
    try
    {
        std::ifstream f(DataDirectory() / "best-score.txt");
        f >> m_Best;
        m_Best = std::clamp(m_Best, 0, 10000000);
    }
    catch (...)
    {
        m_Best = 0;
    }
    DefaultScene();
    if (m_Options.smoke)
    {
        if (m_Options.studio)
        {
            m_View = 1;
            TogglePreview();
        }
        else
            StartMission();
    }
    std::cout << "Skybound renderer: " << m_Renderer.GetBackendName()
              << " | audio=" << (m_AudioReady ? "ready" : "unavailable") << '\n';
}
void SkyboundLayer::OnDetach()
{
    vx::Audio::Shutdown();
    m_Jet.reset();
    m_Ocean.reset();
    m_Renderer.Shutdown();
    m_Initialized = false;
}
void SkyboundLayer::StartMission()
{
    m_Game.Start(m_Difficulty);
    m_Particles.clear();
    m_FocusPaused = false;
    vx::Audio::SetEngineRunning(true);
}
void SkyboundLayer::SwitchView(int view)
{
    if (view == m_View)
        return;
    if (m_Game.state == skybound::State::Playing)
        m_Game.Pause();
    vx::Audio::SetEngineRunning(false);
    m_View = view;
    if (view != 1)
        m_RunningPreview = false;
}
void SkyboundLayer::Burst(vx::Vec2 p, vx::Color c, int count, float spread)
{
    vx::ParticleSettings settings;
    settings.position = p;
    settings.velocitySpread = {spread, spread};
    settings.lifetime = .65f;
    settings.size = 4;
    settings.color = {c.R, c.G, c.B, 1};
    m_Particles.emit(count, settings);
}
void SkyboundLayer::OnUpdate(float dt)
{
    m_Time += dt;
    m_MessageTime = std::max(0.f, m_MessageTime - dt);
    m_Shake = std::max(0.f, m_Shake - dt * 20);
    if (!m_Options.smoke && !vx::Input::HasFocus() && m_Game.state == skybound::State::Playing)
    {
        m_Game.Pause();
        vx::Audio::SetEngineRunning(false);
        m_FocusPaused = true;
    }
    if (m_View == 0)
    {
        skybound::Controls controls{MoveAxis(), vx::Input::IsKeyDown(VK_SPACE)};
        if (m_Options.smoke)
        {
            const float delta = m_Game.enemy.position.y - m_Game.player.position.y;
            controls = {{0, std::clamp(delta / 40, -1.f, 1.f)}, true};
        }
        m_Game.Update(dt, controls);
        if (m_Game.state != skybound::State::Paused)
            m_Particles.update(dt);
        for (const auto& e : m_Game.events)
        {
            using K = skybound::GameEvent::Kind;
            if (e.kind == K::Shot)
            {
                vx::Audio::Play(e.player ? vx::SoundEffect::MachineGun : vx::SoundEffect::EnemyGun);
                Burst(e.position, e.player ? Cyan : Orange, 3, 35);
            }
            if (e.kind == K::Hit)
            {
                vx::Audio::Play(vx::SoundEffect::Hit);
                Burst(e.position, e.player ? Cyan : Orange, 22);
                m_Shake = e.player ? 1.5f : 4;
                m_SmokeHits++;
            }
            if (e.kind == K::Won || e.kind == K::Lost)
            {
                vx::Audio::SetEngineRunning(false);
                vx::Audio::Play(e.kind == K::Won ? vx::SoundEffect::Victory
                                                 : vx::SoundEffect::Explosion);
                Burst(e.position, Orange, 110, 320);
                if (e.kind == K::Won && !m_Options.smoke && m_Game.Score() > m_Best)
                {
                    m_Best = m_Game.Score();
                    try
                    {
                        std::ofstream(DataDirectory() / "best-score.txt") << m_Best;
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
    }
    else if (m_View == 1 && m_RunningPreview)
    {
        m_Preview.update(dt);
    }
}
bool SkyboundLayer::Hit(float x, float y, float w, float h) const
{
    return m_MouseX >= x && m_MouseX <= x + w && m_MouseY >= y && m_MouseY <= y + h;
}
void SkyboundLayer::Label(const std::string& text, float x, float y, float size, vx::Color color)
{
    m_Renderer.DrawText(text, x, y, size, color);
}
bool SkyboundLayer::Button(float x, float y, float w, float h, const std::string& label,
                           bool primary, bool active)
{
    bool hover = Hit(x, y, w, h);
    vx::Color bg = primary  ? Orange
                   : active ? vx::Color{.11f, .22f, .27f, 1}
                   : hover  ? vx::Color{.14f, .19f, .23f, 1}
                            : Panel;
    m_Renderer.DrawQuad(x + w / 2, y + h / 2, w, h, bg);
    if (!primary)
        m_Renderer.DrawRect(x + w / 2, y + h / 2, w, h, active ? Cyan : Line, 1);
    const float font = 14;
    const float textW = m_Renderer.MeasureText(label, font);
    Label(label, x + (w - textW) / 2, y + (h - font) / 2 - 1, font,
          primary  ? Ink
          : active ? Cyan
                   : White);
    return hover && vx::Input::IsMouseButtonPressed(0);
}
void SkyboundLayer::DrawHeader()
{
    m_Renderer.DrawQuad(640, 35, 1280, 70, Panel);
    m_Renderer.DrawLine(24, 70, 1256, 70, Line);
    Label("V", 25, 13, 40, Orange);
    Label("VYNIX", 66, 17, 27);
    Label("AI ENGINE  /  2D", 195, 27, 12, Muted);
    if (Button(720, 19, 145, 34, "01  FLIGHT", false, m_View == 0))
        SwitchView(0);
    if (Button(877, 19, 165, 34, "02  SCENE STUDIO", false, m_View == 1))
        SwitchView(1);
    if (Button(1054, 19, 130, 34, "03  ENGINE", false, m_View == 2))
        SwitchView(2);
    if (Button(1200, 19, 55, 34, vx::Audio::IsMuted() ? "MUTE" : "SND"))
    {
        vx::Audio::SetMuted(!vx::Audio::IsMuted());
    }
}
void SkyboundLayer::RenderHealth(float x, float y, float width, const std::string& name, int hp,
                                 vx::Color color)
{
    Label(name, x, y, 12, color);
    Label(std::to_string(hp) + " / 100", x + width - 85, y, 14);
    m_Renderer.DrawQuad(x + width / 2, y + 29, width, 5, {.1f, .19f, .23f, 1});
    float fill = width * hp / 100;
    m_Renderer.DrawQuad(x + fill / 2, y + 29, fill, 5, color);
}
void SkyboundLayer::DrawPlane(const skybound::Plane& plane, bool player)
{
    if (plane.health <= 0)
        return;
    const float direction = player ? 1.f : -1.f;
    const auto c = player ? Cyan : Orange;
    const float x = GX(plane.position.x) + std::sin(m_Time * 100) * m_Shake,
                y = GY(plane.position.y) + std::cos(m_Time * 87) * m_Shake;
    float flame = 20 + std::sin(m_Time * 70) * 7;
    m_Renderer.DrawQuad(x - direction * 48, y - 7, flame, 3, {c.R, c.G, c.B, .65f});
    m_Renderer.DrawQuad(x - direction * 48, y + 7, flame, 3, {c.R, c.G, c.B, .65f});
    float angle = player ? 0.f : 3.14159265f;
    m_Renderer.DrawQuad(x + 8, y + 13, 92, 88, {0, 0, 0, .25f}, m_Jet.get(), angle);
    auto tint = plane.flash > 0 ? vx::Color{1, .45f, .3f, 1}
                : player        ? vx::Color{.82f, 1, 1, 1}
                                : vx::Color{1, .56f, .36f, 1};
    m_Renderer.DrawQuad(x, y, 92, 88, tint, m_Jet.get(), angle);
    m_Renderer.DrawQuad(x, y + 52, 46, 2, {.13f, .2f, .25f, 1});
    float fill = 46.f * plane.health / 100;
    m_Renderer.DrawQuad(x - 23 + fill / 2, y + 52, fill, 2, c);
}
void SkyboundLayer::DrawFlight()
{
    Label("THE VYNIX FLIGHT SERIES", 24, 87, 12, Muted);
    Label("SKYBOUND", 22, 104, 34);
    Label("MISSION 001  /  OPEN WATER", 937, 112, 13, Cyan);
    m_Renderer.SetClipRect(GameX, GameY, GameW, GameH);
    m_Renderer.DrawQuad(GameX + GameW / 2, GameY + GameH / 2, GameW, GameH, {.72f, .84f, .93f, 1},
                        m_Ocean.get());
    for (int i = 1; i < 9; i++)
    {
        float x = GameX + i * 112 - (std::fmod(m_Time * 8, 112));
        m_Renderer.DrawLine(x, GameY, x, GameY + GameH, {.3f, .6f, .7f, .035f});
    }
    DrawPlane(m_Game.player, true);
    DrawPlane(m_Game.enemy, false);
    for (const auto& b : m_Game.bullets)
    {
        float x = GX(b.position.x), y = GY(b.position.y);
        auto c = b.player ? Cyan : Orange;
        float angle = std::atan2(b.velocity.y, b.velocity.x);
        m_Renderer.DrawQuad(x, y, 23, 7, {c.R, c.G, c.B, .2f}, nullptr, angle);
        m_Renderer.DrawQuad(x, y, 15, 2.5f, c, nullptr, angle);
    }
    for (const auto& p : m_Particles.particles())
    {
        float alpha = 1 - p.age / p.lifetime;
        m_Renderer.DrawQuad(GX(p.position.x), GY(p.position.y), p.size, p.size,
                            {p.color[0], p.color[1], p.color[2], alpha});
    }
    m_Renderer.ClearClipRect();
    RenderHealth(46, 169, 250, "FALCON / YOU", m_Game.player.health, Cyan);
    RenderHealth(650, 169, 250, "VIPER / HOSTILE", m_Game.enemy.health, Orange);
    Label(Clock(m_Game.elapsed), 443, 173, 22);
    Label("N 42 16  /  ATLANTIC AIRSPACE", 46, 625, 11, Muted);
    Label("W 031 08", 813, 625, 11, Muted);
    using S = skybound::State;
    if (m_Game.state == S::Ready)
    {
        m_Renderer.DrawQuad(GameX + GameW / 2, GameY + GameH / 2, GameW, GameH,
                            {.01f, .045f, .065f, .43f});
        Label("ONE-ON-ONE AERIAL COMBAT", 70, 258, 13, Cyan);
        Label("OWN THE", 66, 288, 55);
        Label("OPEN SKY.", 66, 343, 55, Orange);
        Label("One rival. 100 health each.", 70, 418, 19);
        Label("Stay moving. Make every shot count.", 70, 444, 17, Muted);
        if (Button(70, 491, 250, 51, "LAUNCH MISSION  >", true))
            StartMission();
        Label("ENTER TO LAUNCH   /   SPACE TO FIRE", 70, 560, 12, Muted);
    }
    else if (m_Game.state != S::Playing)
    {
        m_Renderer.DrawQuad(GameX + GameW / 2, GameY + GameH / 2, GameW, GameH,
                            {.015f, .04f, .06f, .86f});
        const bool paused = m_Game.state == S::Paused, won = m_Game.state == S::Won;
        Label(paused ? "FLIGHT ON HOLD"
              : won  ? "MISSION COMPLETE"
                     : "AIRCRAFT DOWN",
              230, 268, 14, won ? Cyan : Orange);
        Label(paused ? "TAKE A BREATH." : won ? "SKY SECURED." : "FLY ANOTHER DAY.", 228, 304, 40);
        Label(paused ? (m_FocusPaused ? "Paused while the window is out of focus."
                                      : "Your mission is paused.")
              : won  ? "Viper neutralized. Excellent flying."
                     : "Change your flight path and return to the skies.",
              230, 366, 16, Muted);
        if (!paused)
        {
            Label(std::to_string(m_Game.Score()) + " PTS", 230, 412, 23, Cyan);
            Label(std::to_string(m_Game.Accuracy()) + "% ACCURACY", 443, 418, 14);
        }
        if (Button(230, 467, 245, 48, paused ? "RESUME FLIGHT  >" : "FLY AGAIN  >", true))
        {
            if (paused)
            {
                m_Game.Pause();
                m_FocusPaused = false;
                vx::Audio::SetEngineRunning(true);
            }
            else
                StartMission();
        }
        if (Button(488, 467, 170, 48, "MISSION MENU"))
        {
            m_Game = skybound::Game{};
            m_Particles.clear();
            vx::Audio::SetEngineRunning(false);
        }
    }
    m_Renderer.DrawRect(GameX + GameW / 2, GameY + GameH / 2, GameW, GameH, Line);
    m_Renderer.DrawQuad(474, 675, 900, 42, Panel);
    Label("*  FLIGHT SYSTEMS READY", 40, 669, 12, Cyan);
    Label(m_Renderer.GetBackendName(), 296, 669, 11, Muted);
    if (Button(716, 661, 91, 28, m_Game.state == S::Paused ? "RESUME" : "PAUSE"))
    {
        m_Game.Pause();
        vx::Audio::SetEngineRunning(m_Game.state == S::Playing);
    }
    if (Button(817, 661, 91, 28, "RESTART"))
        StartMission();
    Label("W A S D / ARROWS", 26, 720, 13);
    Label("MOVE", 26, 743, 11, Muted);
    Label("SPACE", 260, 720, 13);
    Label("HOLD TO FIRE", 260, 743, 11, Muted);
    Label("P / ESC", 448, 720, 13);
    Label("PAUSE", 448, 743, 11, Muted);
    Label("R", 618, 720, 13);
    Label("RESTART", 618, 743, 11, Muted);
    Label("M", 788, 720, 13);
    Label("SOUND", 788, 743, 11, Muted);
    m_Renderer.DrawQuad(1098, 422, 316, 548, Panel);
    Label("MISSION BRIEFING", 964, 173, 13, Cyan);
    Label("CLEAR THE", 960, 210, 28);
    Label("AIRSPACE.", 960, 242, 28);
    Label("An enemy interceptor is", 964, 292, 15, Muted);
    Label("approaching from the east.", 964, 313, 15, Muted);
    Label("Bring it down. Stay airborne.", 964, 334, 15, Muted);
    m_Renderer.DrawLine(964, 372, 1232, 372, Line);
    Label("STARTING HEALTH", 964, 392, 11, Muted);
    Label("100 HP / AIRCRAFT", 964, 414, 19);
    Label("YOUR CANNON", 964, 456, 11, Muted);
    Label("10 DAMAGE / HIT", 964, 478, 19);
    Label("PILOT DIFFICULTY", 964, 525, 11, Muted);
    const std::string names[3] = {"CADET", "PILOT", "ACE"};
    for (int i = 0; i < 3; i++)
        if (Button(964 + i * 91.f, 550, 83, 35, names[i], false,
                   static_cast<int>(m_Difficulty) == i) &&
            m_Game.state != S::Playing && m_Game.state != S::Paused)
            m_Difficulty = static_cast<skybound::Difficulty>(i);
    Label("Difficulty applies on launch.", 964, 599, 12, Muted);
    Label("PERSONAL BEST", 964, 639, 11, Muted);
    Label(m_Best ? std::to_string(m_Best) + " PTS" : "--", 1120, 635, 21, Cyan);
    Label("LEAD YOUR SHOTS.", 951, 720, 13, Orange);
    Label("Keep moving to evade aimed fire.", 951, 744, 13, Muted);
}

void SkyboundLayer::DefaultScene()
{
    m_Scene.clear();
    m_Undo.clear();
    m_Redo.clear();
    m_RunningPreview = false;
    m_Drag = vx::InvalidEntity;
    auto id = m_Scene.createEntity("Player plane");
    auto* e = m_Scene.get(id);
    e->transform.position = {240, 290};
    e->sprite.resource = "jet";
    e->sprite.size = {112, 100};
    e->sprite.color = {.7f, 1, 1, 1};
    e->body.useGravity = false;
    e->collider = {{0, 0, 85, 65}, true, false};
    e->script.resource = "player";
    m_Selected = id;
    id = m_Scene.createEntity("Bouncing box");
    e = m_Scene.get(id);
    e->transform.position = {850, 270};
    e->sprite.size = {60, 60};
    e->sprite.color = {1, .4f, .2f, 1};
    e->body.velocity = {0, 130};
    e->body.restitution = 1;
    e->collider = {{0, 0, 60, 60}, true, false};
    vx::TileMap map(25, 14, {48, 48});
    for (unsigned x = 0; x < 25; x++)
    {
        map.setSolid(x, 13);
        map.setSolid(x, 0);
    }
    for (unsigned y = 1; y < 13; y++)
    {
        map.setSolid(0, y);
        map.setSolid(24, y);
    }
    m_Scene.addTileMap(std::move(map));
}
void SkyboundLayer::Remember()
{
    std::ostringstream data;
    if (m_Scene.save(data))
    {
        m_Undo.push_back(data.str());
        if (m_Undo.size() > 32)
            m_Undo.erase(m_Undo.begin());
        m_Redo.clear();
    }
}
void SkyboundLayer::Undo(bool redo)
{
    if (m_RunningPreview)
        return;
    auto& from = redo ? m_Redo : m_Undo;
    auto& to = redo ? m_Undo : m_Redo;
    if (from.empty())
        return;
    std::ostringstream now;
    m_Scene.save(now);
    std::istringstream previous(from.back());
    if (m_Scene.load(previous))
    {
        to.push_back(now.str());
        from.pop_back();
        m_Drag = vx::InvalidEntity;
        m_Selected = m_Scene.entityIds().empty() ? vx::InvalidEntity : m_Scene.entityIds().front();
    }
}
void SkyboundLayer::AddObject(bool plane)
{
    if (m_RunningPreview)
        return;
    if (m_Scene.entityCount() >= 10)
    {
        m_Message = "This studio supports 10 objects per scene.";
        m_MessageTime = 4;
        return;
    }
    Remember();
    auto id = m_Scene.createEntity(plane ? "Plane " + std::to_string(m_Scene.entityCount())
                                         : "Box " + std::to_string(m_Scene.entityCount()));
    auto* e = m_Scene.get(id);
    e->transform.position = {500 + float(m_Scene.entityCount() % 4) * 65, 300};
    e->sprite.resource = plane ? "jet" : "";
    e->sprite.size = plane ? vx::Vec2{100, 92} : vx::Vec2{60, 60};
    e->sprite.color = plane ? vx::SceneColor{.7f, 1, 1, 1} : vx::SceneColor{.4f, .8f, .6f, 1};
    e->body.dynamic = false;
    e->collider = {{0, 0, e->sprite.size.x, e->sprite.size.y}, true, false};
    m_Selected = id;
}
void SkyboundLayer::BindScripts(vx::Scene& scene)
{
    for (auto id : scene.entityIds())
    {
        auto* e = scene.get(id);
        if (e->script.resource == "player")
            e->script.onUpdate = [](vx::Scene& s, vx::EntityId id, float)
            {
                auto* player = s.get(id);
                if (!player)
                    return;
                auto axis = MoveAxis();
                const float n = std::max(1.f, std::hypot(axis.x, axis.y));
                player->body.velocity.x = axis.x * 280 / n;
                if (!player->body.useGravity)
                    player->body.velocity.y = axis.y * 280 / n;
            };
    }
}
void SkyboundLayer::TogglePreview()
{
    if (m_RunningPreview)
    {
        m_RunningPreview = false;
        return;
    }
    std::ostringstream data;
    m_Scene.save(data);
    std::istringstream copy(data.str());
    if (m_Preview.load(copy))
    {
        BindScripts(m_Preview);
        m_RunningPreview = true;
        m_Drag = vx::InvalidEntity;
    }
}
void SkyboundLayer::SaveScene()
{
    try
    {
        auto path = DataDirectory() / "my-scene.vynix";
        auto temporary = path;
        temporary += ".tmp";
        std::ofstream out(temporary, std::ios::trunc);
        bool ok = m_Scene.save(out);
        out.close();
        if (!ok || out.fail())
            throw std::runtime_error("Scene could not be written.");
        if (!MoveFileExW(temporary.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Scene could not be saved.");
        m_Message = "Saved to LocalAppData / VyNixAIEngine / my-scene.vynix";
    }
    catch (const std::exception& e)
    {
        m_Message = e.what();
    }
    m_MessageTime = 5;
}
void SkyboundLayer::LoadScene()
{
    if (m_RunningPreview)
        return;
    try
    {
        std::ifstream in(DataDirectory() / "my-scene.vynix");
        std::string error;
        std::ostringstream old;
        m_Scene.save(old);
        if (m_Scene.load(in, &error))
        {
            m_Undo.push_back(old.str());
            if (m_Undo.size() > 32)
                m_Undo.erase(m_Undo.begin());
            m_Redo.clear();
            m_Drag = vx::InvalidEntity;
            m_Selected =
                m_Scene.entityIds().empty() ? vx::InvalidEntity : m_Scene.entityIds().front();
            m_Message = "Saved scene loaded.";
        }
        else
            m_Message = "Load failed: " + error;
    }
    catch (const std::exception& e)
    {
        m_Message = e.what();
    }
    m_MessageTime = 5;
}
void SkyboundLayer::DrawScene(vx::Scene& scene)
{
    m_Renderer.SetClipRect(StudioX, StudioY, StudioW, StudioH);
    m_Renderer.DrawQuad(StudioX + StudioW / 2, StudioY + StudioH / 2, StudioW, StudioH,
                        {.04f, .09f, .12f, 1});
    const float sx = StudioW / 1200, sy = StudioH / 675;
    for (int x = 0; x <= 1200; x += 48)
        m_Renderer.DrawLine(StudioX + x * sx, StudioY, StudioX + x * sx, StudioY + StudioH,
                            {.11f, .2f, .24f, 1});
    for (int y = 0; y <= 675; y += 48)
        m_Renderer.DrawLine(StudioX, StudioY + y * sy, StudioX + StudioW, StudioY + y * sy,
                            {.11f, .2f, .24f, 1});
    for (const auto& map : scene.tileMaps())
        for (const auto& r : map.colliders())
            m_Renderer.DrawQuad(StudioX + r.x * sx, StudioY + r.y * sy, r.w * sx - 1, r.h * sy - 1,
                                {.14f, .26f, .3f, 1});
    vx::EntityId clicked = vx::InvalidEntity;
    for (auto id : scene.entityIds())
    {
        const auto* e = scene.get(id);
        if (!e->active || !e->sprite.visible)
            continue;
        const auto& p = e->transform.position;
        const auto& c = e->sprite.color;
        float w = e->sprite.size.x * e->transform.scale.x * sx,
              h = e->sprite.size.y * e->transform.scale.y * sy;
        float x = StudioX + p.x * sx, y = StudioY + p.y * sy;
        m_Renderer.DrawQuad(x, y, w, h, {c[0], c[1], c[2], c[3]},
                            e->sprite.resource == "jet" ? m_Jet.get() : nullptr,
                            e->transform.rotation);
        if (id == m_Selected && !m_RunningPreview)
        {
            m_Renderer.DrawRect(x, y, w + 8, h + 8, Cyan, 1.5f);
            m_Renderer.DrawQuad(x - w / 2 - 4, y - h / 2 - 4, 5, 5, Cyan);
            m_Renderer.DrawQuad(x + w / 2 + 4, y + h / 2 + 4, 5, 5, Cyan);
        }
        if (!m_RunningPreview && vx::Input::IsMouseButtonPressed(0) &&
            Hit(x - w / 2, y - h / 2, w, h))
            clicked = id;
    }
    if (clicked)
    {
        m_Selected = clicked;
        m_Drag = clicked;
        Remember();
        m_DragOffset = scene.get(clicked)->transform.position -
                       vx::Vec2{(m_MouseX - StudioX) / sx, (m_MouseY - StudioY) / sy};
    }
    if (!m_RunningPreview && m_Drag && vx::Input::IsMouseButtonDown(0) &&
        Hit(StudioX, StudioY, StudioW, StudioH))
    {
        if (auto* e = scene.get(m_Drag))
        {
            e->transform.position = {
                std::clamp((m_MouseX - StudioX) / sx + m_DragOffset.x, 55.f, 1145.f),
                std::clamp((m_MouseY - StudioY) / sy + m_DragOffset.y, 55.f, 615.f)};
        }
    }
    if (!vx::Input::IsMouseButtonDown(0))
        m_Drag = vx::InvalidEntity;
    m_Renderer.ClearClipRect();
    m_Renderer.DrawRect(StudioX + StudioW / 2, StudioY + StudioH / 2, StudioW, StudioH, Line);
}
void SkyboundLayer::DrawStudio()
{
    Label("BUILD WITH VYNIX", 24, 88, 12, Muted);
    Label("SCENE STUDIO", 22, 105, 30);
    if (Button(601, 99, 90, 34, "NEW"))
    {
        std::ostringstream previous;
        m_Scene.save(previous);
        DefaultScene();
        m_Undo.push_back(previous.str());
        m_Message = "New scene. Undo restores the previous scene.";
        m_MessageTime = 4;
    }
    if (Button(701, 99, 88, 34, "LOAD"))
        LoadScene();
    if (Button(799, 99, 88, 34, "SAVE"))
        SaveScene();
    if (Button(898, 99, 96, 34, m_RunningPreview ? "STOP" : "PLAY  >", true))
        TogglePreview();
    m_Renderer.DrawQuad(121, 386, 194, 456, Panel);
    Label("SCENE OBJECTS", 40, 174, 12, Muted);
    if (Button(39, 203, 76, 32, "+ PLANE"))
        AddObject(true);
    if (Button(125, 203, 77, 32, "+ BOX"))
        AddObject(false);
    int index = 0;
    for (auto id : m_Scene.entityIds())
    {
        const auto* e = m_Scene.get(id);
        if (index < 10 && Button(39, 249 + index * 31.f, 163, 27, e->name, false, id == m_Selected))
            m_Selected = id;
        index++;
    }
    if (Button(39, 578, 76, 25, "UNDO"))
        Undo();
    if (Button(125, 578, 77, 25, "REDO"))
        Undo(true);
    DrawScene(m_RunningPreview ? m_Preview : m_Scene);
    Label(m_RunningPreview ? "PLAYING  /  WASD OR ARROWS MOVE THE PLAYER"
                           : "EDITING  /  DRAG AN OBJECT TO MOVE IT",
          239, 604, 12, m_RunningPreview ? Cyan : Muted);
    Label("1200 x 675", 884, 604, 12, Muted);
    m_Renderer.DrawQuad(1133, 397, 246, 478, Panel);
    Label("OBJECT INSPECTOR", 1029, 174, 12, Muted);
    auto* selected = m_Scene.get(m_Selected);
    if (selected)
    {
        Label(selected->name, 1029, 205, 18, Cyan);
        auto adjust = [&](const std::string& label, float value, float y, auto callback)
        {
            Label(label, 1029, y + 8, 12, Muted);
            Label(Number(value), 1111, y + 6, 15);
            if (Button(1171, y, 28, 29, "-") && !m_RunningPreview)
            {
                Remember();
                callback(-1);
            }
            if (Button(1207, y, 28, 29, "+") && !m_RunningPreview)
            {
                Remember();
                callback(1);
            }
        };
        adjust("X", selected->transform.position.x, 240,
               [&](int d)
               {
                   selected->transform.position.x =
                       std::clamp(selected->transform.position.x + d * 10, 50.f, 1150.f);
               });
        adjust("Y", selected->transform.position.y, 280,
               [&](int d)
               {
                   selected->transform.position.y =
                       std::clamp(selected->transform.position.y + d * 10, 50.f, 615.f);
               });
        adjust("SIZE", selected->sprite.size.x, 320,
               [&](int d)
               {
                   float next = std::clamp(selected->sprite.size.x + d * 10, 10.f, 200.f),
                         ratio = next / selected->sprite.size.x;
                   selected->sprite.size.x = next;
                   selected->sprite.size.y *= ratio;
                   selected->collider.bounds.w = next;
                   selected->collider.bounds.h = selected->sprite.size.y;
               });
        adjust("SPEED X", selected->body.velocity.x, 360,
               [&](int d)
               {
                   selected->body.velocity.x =
                       std::clamp(selected->body.velocity.x + d * 25, -500.f, 500.f);
               });
        adjust("SPEED Y", selected->body.velocity.y, 400,
               [&](int d)
               {
                   selected->body.velocity.y =
                       std::clamp(selected->body.velocity.y + d * 25, -500.f, 500.f);
               });
        if (Button(1029, 445, 206, 30, selected->body.dynamic ? "BODY: DYNAMIC" : "BODY: STATIC",
                   false, selected->body.dynamic) &&
            !m_RunningPreview)
        {
            Remember();
            selected->body.dynamic = !selected->body.dynamic;
        }
        if (Button(1029, 484, 98, 30, selected->body.useGravity ? "GRAVITY ON" : "GRAVITY OFF",
                   false, selected->body.useGravity) &&
            !m_RunningPreview)
        {
            Remember();
            selected->body.useGravity = !selected->body.useGravity;
        }
        if (Button(1137, 484, 98, 30, selected->collider.enabled ? "COLLIDE ON" : "COLLIDE OFF",
                   false, selected->collider.enabled) &&
            !m_RunningPreview)
        {
            Remember();
            selected->collider.enabled = !selected->collider.enabled;
        }
        if (Button(1029, 523, 206, 30,
                   selected->script.resource == "player" ? "CONTROL: PLAYER" : "CONTROL: VELOCITY",
                   false, selected->script.resource == "player") &&
            !m_RunningPreview)
        {
            Remember();
            selected->script.resource = selected->script.resource == "player" ? "" : "player";
            if (selected->script.resource == "player")
                selected->body.dynamic = true;
        }
        if (Button(1029, 579, 206, 30, "DELETE OBJECT") && !m_RunningPreview)
        {
            Remember();
            m_Scene.destroyEntity(m_Selected);
            m_Selected = vx::InvalidEntity;
        }
    }
    else
        Label("Select or add an object.", 1029, 216, 15, Muted);
    m_Renderer.DrawLine(24, 654, 1256, 654, Line);
    Label("MAKE SOMETHING MOVE.", 24, 679, 18);
    Label("1  Add a plane or box.", 24, 718, 15, Muted);
    Label("2  Set velocity, gravity, or player control.", 326, 718, 15, Muted);
    Label("3  Press Play to test. Stop to edit.", 779, 718, 15, Muted);
    Label(
        m_MessageTime > 0
            ? m_Message
            : "Save stores your editable scene locally. Ctrl+S saves. Ctrl+Z / Ctrl+Y undo / redo.",
        24, 760, 13, m_MessageTime > 0 ? Cyan : Muted);
}
void SkyboundLayer::DrawEngine()
{
    Label("C++20 / NATIVE OPENGL", 24, 94, 13, Cyan);
    Label("YOUR 2D TOOLKIT.", 21, 120, 44);
    Label("Reusable engine modules. A playable game. A scene you can make your own.", 24, 184, 18,
          Muted);
    const char* titles[6] = {"RENDERER",       "SCENES & SCRIPTS",    "COLLISION & MOTION",
                             "INPUT & WINDOW", "ANIMATION & EFFECTS", "AUDIO & PROJECTS"};
    const char* line1[6] = {"Batched sprites, PNG textures,",   "Entities, transforms, cameras,",
                            "Swept AABB collisions, triggers,", "Keyboard, mouse, focus, resize,",
                            "Sprite frames, tween curves,",     "Machine tones, engine hum,"};
    const char* line2[6] = {
        "atlas UVs, tint, rotation, text.",  "scene switching, C++ callbacks.",
        "tile walls, gravity, restitution.", "fixed-step updates, layer events.",
        "timers and bounded particles.",     "mute, local scene save and load."};
    for (int i = 0; i < 6; i++)
    {
        float x = 24 + (i % 3) * 418.f, y = 239 + (i / 3) * 151.f;
        m_Renderer.DrawQuad(x + 197, y + 65, 394, 130, Panel);
        Label("0" + std::to_string(i + 1), x + 20, y + 17, 12, Orange);
        Label(titles[i], x + 20, y + 43, 18);
        Label(line1[i], x + 20, y + 79, 14, Muted);
        Label(line2[i], x + 20, y + 101, 14, Muted);
    }
    Label("CREATE A SIMPLE GAME", 24, 575, 22);
    Label("Use Scene Studio to arrange a scene, or build a Layer in C++ using the engine modules.",
          24, 615, 17, Muted);
    Label("Open README.md for build steps and the engine overview.", 24, 644, 16, Muted);
    Label("V1 SCOPE", 24, 696, 12, Orange);
    Label("Windows desktop, translational 2D physics, C++ game scripts. No networking or "
          "AI-generation service yet.",
          24, 722, 15, Muted);
    Label("ACTIVE BACKEND: " + m_Renderer.GetBackendName(), 24, 765, 12, Cyan);
}
void SkyboundLayer::OnRender()
{
    if (!m_Initialized)
        return;
    const auto& window = m_App.GetWindow();
    const auto w = window.GetWidth(), h = window.GetHeight();
    m_MouseX = vx::Input::MouseX() * 1280.f / std::max(1u, w);
    m_MouseY = vx::Input::MouseY() * 800.f / std::max(1u, h);
    if (vx::Input::IsKeyPressed(VK_F1))
        SwitchView(0);
    if (vx::Input::IsKeyPressed(VK_F2))
        SwitchView(1);
    if (vx::Input::IsKeyPressed(VK_F3))
        SwitchView(2);
    if (vx::Input::IsKeyPressed('M'))
        vx::Audio::SetMuted(!vx::Audio::IsMuted());
    if (m_View == 0)
    {
        if (vx::Input::IsKeyPressed(VK_RETURN))
        {
            if (m_Game.state == skybound::State::Paused)
            {
                m_Game.Pause();
                vx::Audio::SetEngineRunning(true);
            }
            else if (m_Game.state != skybound::State::Playing)
                StartMission();
        }
        if (vx::Input::IsKeyPressed('P') || vx::Input::IsKeyPressed(VK_ESCAPE))
        {
            m_Game.Pause();
            vx::Audio::SetEngineRunning(m_Game.state == skybound::State::Playing);
        }
        if (vx::Input::IsKeyPressed('R'))
            StartMission();
    }
    if (m_View == 1)
    {
        if (vx::Input::IsKeyPressed(VK_SPACE))
            TogglePreview();
        if (vx::Input::IsKeyDown(VK_CONTROL))
        {
            if (vx::Input::IsKeyPressed('S'))
                SaveScene();
            if (vx::Input::IsKeyPressed('Z'))
                Undo();
            if (vx::Input::IsKeyPressed('Y'))
                Undo(true);
        }
    }
    m_Renderer.BeginFrame(static_cast<int>(w), static_cast<int>(h), 1280, 800, Ink);
    DrawHeader();
    if (m_View == 0)
        DrawFlight();
    else if (m_View == 1)
        DrawStudio();
    else
        DrawEngine();
    m_Renderer.EndFrame();
    m_Frame++;
    if (m_Options.smoke && m_Frame == m_Options.frames)
    {
        bool capture = true;
        if (!m_Options.screenshot.empty())
        {
            if (m_Options.screenshot.has_parent_path())
                std::filesystem::create_directories(m_Options.screenshot.parent_path());
            capture = m_Renderer.CapturePPM(m_Options.screenshot);
        }
        bool simulationPassed = m_SmokeHits > 0;
        if (m_Options.studio)
        {
            auto* original = m_Scene.get(2);
            auto* running = m_Preview.get(2);
            simulationPassed = original && running &&
                               original->transform.position != running->transform.position &&
                               m_RunningPreview;
        }
        m_SmokePassed = capture && m_Renderer.GetStats().QuadCount > 20 && simulationPassed;
        std::cout << "SMOKE " << (m_SmokePassed ? "PASS" : "FAIL") << " frames=" << m_Frame
                  << " hits=" << m_SmokeHits << " player_hp=" << m_Game.player.health
                  << " enemy_hp=" << m_Game.enemy.health
                  << " quads=" << m_Renderer.GetStats().QuadCount
                  << " draw_calls=" << m_Renderer.GetStats().DrawCalls << '\n';
    }
}
