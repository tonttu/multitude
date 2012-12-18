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
      ADD                       = GL_FUNC_ADD,
      SUBTRACT                  = GL_FUNC_SUBTRACT,
      REVERSE_SUBTRACT           = GL_FUNC_REVERSE_SUBTRACT,
      MIN                       = GL_MIN,
      MAX                       = GL_MAX
    };

    enum Function
    {
      ZERO                      = GL_ZERO,
      ONE                       = GL_ONE,
      SOURCE_COLOR               = GL_SRC_COLOR,
      SOURCE_ALPHA               = GL_SRC_ALPHA,
      ONE_MINUS_SOURCE_COLOR       = GL_ONE_MINUS_SRC_COLOR,
      ONE_MINUS_SOURCE_ALPHA       = GL_ONE_MINUS_SRC_ALPHA,

      DESTINATION_COLOR          = GL_DST_COLOR,
      DESTINATION_ALPHA          = GL_DST_ALPHA,
      ONE_MINUS_DESTINATION_COLOR  = GL_ONE_MINUS_DST_COLOR,
      ONE_MINUS_DESTINATION_ALPHA  = GL_ONE_MINUS_DST_ALPHA,

      CONSTANT_COLOR             = GL_CONSTANT_COLOR,
      CONSTANT_ALPHA             = GL_CONSTANT_ALPHA,
      ONE_MINUS_CONSTANT_COLOR     = GL_ONE_MINUS_CONSTANT_COLOR,
      ONE_MINUS_CONSTANT_ALPHA     = GL_ONE_MINUS_CONSTANT_ALPHA,

      ALPHA_SATURATE             = GL_SRC_ALPHA_SATURATE,

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

  inline bool operator!=(const BlendMode & lhs, const BlendMode & rhs)
  {
    return
      lhs.equation() != rhs.equation()
      || lhs.sourceFunction() != rhs.sourceFunction()
      || lhs.destFunction() != rhs.destFunction()
      || lhs.constantColor() != rhs.constantColor();
  }

  inline bool operator==(const BlendMode & lhs, const BlendMode & rhs) { return !(lhs!=rhs); }
}
#endif // LUMINOUS_BLENDMODE_HPP
