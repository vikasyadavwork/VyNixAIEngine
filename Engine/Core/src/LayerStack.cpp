#include "vx/Core/LayerStack.h"
#include "vx/Core/Layer.h"

#include <algorithm>

namespace vx
{

LayerStack::~LayerStack()
{
    Clear();
}

void LayerStack::Clear()
{
    for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it)
    {
        Layer* layer = *it;
        layer->OnDetach();
        delete layer;
    }
    m_Layers.clear();
    m_LayerInsertIndex = 0;
}

void LayerStack::PushLayer(Layer* layer)
{
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);

    ++m_LayerInsertIndex;

    layer->OnAttach();
}

void LayerStack::PushOverlay(Layer* overlay)
{
    m_Layers.emplace_back(overlay);

    overlay->OnAttach();
}

void LayerStack::PopLayer(Layer* layer)
{
    auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);

    if (it == m_Layers.begin() + m_LayerInsertIndex)
        return;

    (*it)->OnDetach();

    m_Layers.erase(it);

    --m_LayerInsertIndex;
}

void LayerStack::PopOverlay(Layer* overlay)
{
    auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);

    if (it == m_Layers.end())
        return;

    (*it)->OnDetach();

    m_Layers.erase(it);
}

} // namespace vx
