#include "Luminous/DepthMode.hpp"

namespace Luminous
{
  DepthMode::DepthMode()
    : m_function(LESS)
    , m_range(0.f, 1.f)
  {
  }
}