#include "Luminous/DepthMode.hpp"

namespace Luminous
{
  DepthMode::DepthMode()
    : m_function(Less)
    , m_range(0.f, 1.f)
  {
  }
}