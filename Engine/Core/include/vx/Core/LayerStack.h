#pragma once

#include <vector>

namespace vx
{

class Layer;

class LayerStack
{
  public:
    LayerStack() = default;
    ~LayerStack();
    void Clear();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    auto begin() noexcept
    {
        return m_Layers.begin();
    }
    auto end() noexcept
    {
        return m_Layers.end();
    }

    auto begin() const noexcept
    {
        return m_Layers.begin();
    }
    auto end() const noexcept
    {
        return m_Layers.end();
    }
    auto rbegin() noexcept
    {
        return m_Layers.rbegin();
    }
    auto rend() noexcept
    {
        return m_Layers.rend();
    }

  private:
    std::vector<Layer*> m_Layers;
    size_t m_LayerInsertIndex = 0;
};

} // namespace vx
