#pragma once
#include "SkyboundGame.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <vx/Core/Application.h>
#include <vx/Renderer/Renderer2D.h>
#include <vx/Scene/Scene.h>

struct LaunchOptions
{
    bool smoke = false;
    bool studio = false;
    int frames = 360;
    std::filesystem::path screenshot;
};

class SkyboundLayer final : public vx::Layer
{
  public:
    SkyboundLayer(vx::Application& app, LaunchOptions options);
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    bool SmokePassed() const
    {
        return m_SmokePassed;
    }

  private:
    void DrawHeader();
    void DrawFlight();
    void DrawStudio();
    void DrawEngine();
    void DrawPlane(const skybound::Plane& plane, bool player);
    void DrawScene(vx::Scene& scene);
    bool Button(float x, float y, float w, float h, const std::string& label, bool primary = false,
                bool active = false);
    void Label(const std::string& text, float x, float y, float size = 16,
               vx::Color color = {.87f, .92f, .95f, 1});
    bool Hit(float x, float y, float w, float h) const;
    void StartMission();
    void SwitchView(int view);
    void DefaultScene();
    void AddObject(bool plane);
    void Remember();
    void Undo(bool redo = false);
    void SaveScene();
    void LoadScene();
    void TogglePreview();
    void BindScripts(vx::Scene& scene);
    void RenderHealth(float x, float y, float width, const std::string& name, int hp,
                      vx::Color color);
    void Burst(vx::Vec2 p, vx::Color c, int count = 20, float spread = 180);
    std::filesystem::path DataDirectory() const;
    vx::Application& m_App;
    LaunchOptions m_Options;
    vx::Renderer2D m_Renderer;
    std::shared_ptr<vx::Texture2D> m_Jet, m_Ocean;
    skybound::Game m_Game;
    vx::ParticleEmitter m_Particles{700, 19};
    vx::Scene m_Scene, m_Preview;
    vx::EntityId m_Selected = vx::InvalidEntity, m_Drag = vx::InvalidEntity;
    vx::Vec2 m_DragOffset{};
    std::vector<std::string> m_Undo, m_Redo;
    int m_View = 0, m_Frame = 0, m_Best = 0;
    skybound::Difficulty m_Difficulty = skybound::Difficulty::Pilot;
    bool m_RunningPreview = false, m_SmokePassed = false, m_AudioReady = false,
         m_FocusPaused = false, m_Initialized = false;
    float m_Time = 0, m_MessageTime = 0, m_Shake = 0, m_MouseX = 0, m_MouseY = 0;
    std::string m_Message;
    int m_SmokeHits = 0;
};
