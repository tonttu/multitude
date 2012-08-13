#include "Luminous/BlendMode.hpp"

namespace Luminous
{
  BlendMode::BlendMode()
    : m_color(0,0,0,0)
    , m_equation(Equation::Add)
    , m_srcFunction(Function::One)
    , m_dstFunction(Function::Zero)
  {
  }

  BlendMode BlendMode::Additive()
  {
    BlendMode mode;
    mode.setSourceFunction(Function::SourceAlpha);
    mode.setDestFunction(Function::One);
    mode.setEquation(Equation::Add);
    return mode;
  }

  BlendMode BlendMode::Subtractive()
  {
    BlendMode mode;
    mode.setSourceFunction(Function::SourceAlpha);
    mode.setDestFunction(Function::OneMinusSourceAlpha);
    mode.setEquation(Equation::ReverseSubtract);
    return mode;
  }
}