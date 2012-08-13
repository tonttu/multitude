#if !defined(LUMINOUS_BLENDMODE_HPP)
#define LUMINOUS_BLENDMODE_HPP

#include <Radiant/Color.hpp>
#include <Luminous/Luminous.hpp>

namespace Luminous
{
  class BlendMode
  {
  public:
    enum Equation
    {
      Add                       = GL_FUNC_ADD,
      Subtract                  = GL_FUNC_SUBTRACT,
      ReverseSubtract           = GL_FUNC_REVERSE_SUBTRACT,
      Min                       = GL_MIN,
      Max                       = GL_MAX,
    };

    enum Function
    {
      Zero                      = GL_ZERO,
      One                       = GL_ONE,
      SourceColor               = GL_SRC_COLOR,
      SourceAlpha               = GL_SRC_ALPHA,
      OneMinusSourceColor       = GL_ONE_MINUS_SRC_COLOR,
      OneMinusSourceAlpha       = GL_ONE_MINUS_SRC_ALPHA,

      DestinationColor          = GL_DST_COLOR,
      DestinationAlpha          = GL_DST_ALPHA,
      OneMinusDestinationColor  = GL_ONE_MINUS_DST_COLOR,
      OneMinusDestinationAlpha  = GL_ONE_MINUS_DST_ALPHA,

      ConstantColor             = GL_CONSTANT_COLOR,
      ConstantAlpha             = GL_CONSTANT_ALPHA,
      OneMinusConstantColor     = GL_ONE_MINUS_CONSTANT_COLOR,
      OneMinusConstantAlpha     = GL_ONE_MINUS_CONSTANT_ALPHA,

      AlphaSaturate             = GL_SRC_ALPHA_SATURATE,

      // GL_SRC1_COLOR
      // GL_ONE_MINUS_SRC1_COLOR
      // GL_SRC1_ALPHA
      // GL_ONE_MINUS_SRC1_ALPHA
    };

  public:
    static BlendMode Default() { return BlendMode(); }

    LUMINOUS_API static BlendMode Additive();
    LUMINOUS_API static BlendMode Subtractive();

    LUMINOUS_API BlendMode();

    void setConstantColor(const Radiant::Color & color) { m_color = color; }
    const Radiant::Color & constantColor() const { return m_color; }

    void setEquation(Equation eq) { m_equation = eq; }
    Equation equation() const { return m_equation; }

    void setSourceFunction(Function func) { m_srcFunction = func; }
    void setDestFunction(Function func) { m_dstFunction = func; }
    Function sourceFunction() const { return m_srcFunction; }
    Function destFunction() const { return m_dstFunction; }

  private:
    Radiant::Color m_color;
    Equation m_equation;
    Function m_srcFunction;
    Function m_dstFunction;
  };
}
#endif // LUMINOUS_BLENDMODE_HPP