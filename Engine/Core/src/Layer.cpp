#include "vx/Core/Layer.h"

#include <utility>

namespace vx
{

Layer::Layer(std::string name) : m_Name(std::move(name)) {}

} // namespace vx