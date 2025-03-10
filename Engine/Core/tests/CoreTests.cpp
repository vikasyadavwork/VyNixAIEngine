#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <vx/Core/Input.h>
#include <vx/Core/Layer.h>
#include <vx/Core/LayerStack.h>
#include <vx/Core/Timer.h>

namespace
{
void Require(bool condition, const char* reason)
{
    if (!condition)
        throw std::runtime_error(reason);
}
struct RecordedLayer final : vx::Layer
{
    RecordedLayer(int id, std::vector<int>& trace) : id(id), trace(trace) {}
    void OnDetach() override
    {
        trace.push_back(id);
    }
    int id;
    std::vector<int>& trace;
};
} // namespace

int main()
{
    try
    {
        vx::Input::Reset();
        vx::Input::SetFocus(true);
        vx::Input::SetKeyDown('W', true);
        Require(vx::Input::IsKeyDown('W') && vx::Input::IsKeyPressed('W'),
                "Initial key press missing");
        vx::Input::EndFrame();
        Require(vx::Input::IsKeyDown('W') && !vx::Input::IsKeyPressed('W'), "Held key retriggered");
        vx::Input::SetKeyDown('W', true);
        Require(!vx::Input::IsKeyPressed('W'), "OS auto-repeat became a fresh press");
        vx::Input::SetKeyDown('W', false);
        Require(vx::Input::IsKeyReleased('W') && !vx::Input::IsKeyDown('W'), "Release missing");
        vx::Input::SetKeyDown(-1, true);
        Require(!vx::Input::IsKeyDown(-1) && !vx::Input::IsKeyPressed(300),
                "Invalid key not rejected");
        vx::Input::SetMousePosition(42, 18);
        vx::Input::SetMouseButtonDown(0, true);
        vx::Input::AddScroll(1);
        Require(vx::Input::MouseX() == 42 && vx::Input::MouseY() == 18, "Mouse position missing");
        Require(vx::Input::IsMouseButtonPressed(0) && vx::Input::ScrollDelta() == 1,
                "Mouse transition missing");
        vx::Input::SetKeyDown('D', true);
        vx::Input::SetFocus(false);
        Require(!vx::Input::HasFocus() && !vx::Input::IsKeyDown('D') &&
                    !vx::Input::IsMouseButtonDown(0),
                "Focus loss left held input");
        Require(!vx::Input::IsMouseButtonPressed(0) && vx::Input::ScrollDelta() == 0,
                "Focus loss left stale edges");

        vx::Timer timer;
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
        timer.Update();
        Require(timer.GetDeltaTime() > .001f && timer.GetDeltaTime() < 2.0f,
                "Timer lost precision in absolute timestamp");
        const float first = timer.GetElapsedTime();
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        timer.Update();
        Require(timer.GetElapsedTime() > first, "Timer did not advance");

        std::vector<int> detachOrder;
        {
            vx::LayerStack layers;
            layers.PushLayer(new RecordedLayer(1, detachOrder));
            layers.PushOverlay(new RecordedLayer(2, detachOrder));
            layers.Clear();
            Require(detachOrder == std::vector<int>({2, 1}),
                    "Layers did not detach in reverse order");
            layers.Clear();
        }
        Require(detachOrder.size() == 2, "Layer clear detached twice");
        std::cout << "Core input, timer, and lifecycle checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
