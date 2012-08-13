#include "Luminous/BlendMode.hpp"

namespace Luminous
{
  BlendMode::BlendMode()
    : m_color(0,0,0,0)
    , m_equation(Add)
    , m_srcFunction(SourceAlpha)
    , m_dstFunction(OneMinusSourceAlpha)
  {
  }

  BlendMode BlendMode::Additive()
  {
    BlendMode mode;
    mode.setSourceFunction(SourceAlpha);
    mode.setDestFunction(One);
    mode.setEquation(Add);
    return mode;
  }

  BlendMode BlendMode::Subtractive()
  {
    BlendMode mode;
    mode.setSourceFunction(SourceAlpha);
    mode.setDestFunction(OneMinusSourceAlpha);
    mode.setEquation(ReverseSubtract);
    return mode;
  }
}
