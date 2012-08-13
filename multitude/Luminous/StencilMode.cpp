#include "Luminous/StencilMode.hpp"

namespace Luminous
{
  StencilMode::StencilMode()
    : m_function(Always)
    , m_stencilFail(Keep)
    , m_depthFail(Keep)
    , m_pass(Keep)
    , m_refValue(0)
    , m_maskValue(0xffffffff)
  {
  }
}