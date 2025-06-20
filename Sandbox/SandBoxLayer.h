#pragma once
#include <vx/Core/Layer.h>
#include <vx/Core/Log.h>

class SandboxLayer : public vx::Layer
{
  public:
    SandboxLayer() : Layer("Sandbox") {}

    void OnAttach() override
    {
        VX_LOG_INFO("Sandbox Attached");
    }

    void OnDetach() override
    {
        VX_LOG_INFO("Sandbox Detached");
    }

    void OnUpdate(float dt) override
    {
        // Temporary
        // VX_LOG_INFO("Updating Frame");
    }
};
